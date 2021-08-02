//
//  YoutubeMediaProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/23/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "YoutubeMediaProvider.hpp"
#include "api/YoutubeError.hpp"
#include "mutators/YoutubePlaylistMutatorDelegate.hpp"
#include <soundhole/utils/SoundHoleError.hpp>
#include <soundhole/utils/Utils.hpp>

namespace sh {
	YoutubeMediaProvider::YoutubeMediaProvider(Options options, StreamPlayer* streamPlayer)
	: youtube(new Youtube({
		.auth = options.auth
	})),
	_player(new YoutubePlaybackProvider(this, streamPlayer)),
	libraryPlaylistName(options.libraryPlaylistName),
	libraryPlaylistDescription(options.libraryPlaylistDescription) {
		//
	}

	YoutubeMediaProvider::~YoutubeMediaProvider() {
		delete _player;
		delete youtube;
	}
	
	String YoutubeMediaProvider::name() const {
		return NAME;
	}
	
	String YoutubeMediaProvider::displayName() const {
		return "YouTube";
	}




	Youtube* YoutubeMediaProvider::api() {
		return youtube;
	}

	const Youtube* YoutubeMediaProvider::api() const {
		return youtube;
	}



	#pragma mark URI/URL parsing

	YoutubeMediaProvider::URI YoutubeMediaProvider::parseURI(String uri) const {
		auto uriParts = ArrayList<String>(uri.split(':'));
		if(uriParts.size() != 3 || uriParts.containsWhere([](auto& part) { return part.empty(); }) || uriParts.front() != name()) {
			throw SoundHoleError(SoundHoleError::Code::PARSE_FAILED, "invalid youtube uri "+uri);
		}
		return URI{
			.provider=name(),
			.type=uriParts[1],
			.id=uriParts[2]
		};
	}

	YoutubeMediaProvider::URI YoutubeMediaProvider::parseURL(String url) const {
		auto urlObj = Url(url);
		if(urlObj.host() != "www.youtube.com" && urlObj.host() != "youtube.com" && urlObj.host() != "youtu.be" && urlObj.host() != "www.youtu.be") {
			throw SoundHoleError(SoundHoleError::Code::PARSE_FAILED, "invalid youtube URL");
		}
		String path = urlObj.path();
		String channelStarter = "/channel/";
		if(path.startsWith(channelStarter)) {
			auto channelId = path.substring(channelStarter.length());
			while(channelId.startsWith("/")) {
				channelId = channelId.substring(1);
			}
			auto slashIndex = channelId.indexOf('/');
			if(slashIndex != -1) {
				channelId = channelId.substring(0, slashIndex);
			}
			if(channelId.empty()) {
				throw SoundHoleError(SoundHoleError::Code::PARSE_FAILED, "invalid youtube channel URL "+url);
			}
			return URI{
				.provider=name(),
				.type="channel",
				.id=channelId
			};
		}
		String userStarter = "/user/";
		if(path.startsWith(userStarter)) {
			auto userId = path.substring(userStarter.length());
			while(userId.startsWith("/")) {
				userId = userId.substring(1);
			}
			auto slashIndex = userId.indexOf('/');
			if(slashIndex != -1) {
				userId = userId.substring(0, slashIndex);
			}
			if(userId.empty()) {
				throw SoundHoleError(SoundHoleError::Code::PARSE_FAILED, "invalid youtube user URL "+url);
			}
			return URI{
				.provider=name(),
				.type="user",
				.id=userId
			};
		}
		String videoStarter = "/v/";
		if(path.startsWith(videoStarter)) {
			auto videoId = path.substring(videoStarter.length());
			while(videoId.startsWith("/")) {
				videoId = videoId.substring(1);
			}
			auto slashIndex = videoId.indexOf('/');
			if(slashIndex != -1) {
				videoId = videoId.substring(0, slashIndex);
			}
			if(videoId.empty()) {
				throw SoundHoleError(SoundHoleError::Code::PARSE_FAILED, "invalid youtube video URL "+url);
			}
			return URI{
				.provider=name(),
				.type="video",
				.id=videoId
			};
		}
		if(path.empty() || path == "/" || path == "/watch" || path == "/watch/") {
			String videoId;
			for(auto& queryItem : urlObj.query()) {
				if(queryItem.key() == "v") {
					videoId = queryItem.val();
					break;
				}
			}
			if(videoId.empty()) {
				throw SoundHoleError(SoundHoleError::Code::PARSE_FAILED, "invalid youtube video URL "+url);
			}
			return URI{
				.provider=name(),
				.type="video",
				.id=videoId
			};
		}
		if(path == "/playlist" || path=="/playlist") {
			String playlistId;
			for(auto& queryItem : urlObj.query()) {
				if(queryItem.key() == "list") {
					playlistId = queryItem.val();
					break;
				}
			}
			if(playlistId.empty()) {
				throw SoundHoleError(SoundHoleError::Code::PARSE_FAILED, "invalid youtube playlist URL "+url);
			}
			return URI{
				.provider=name(),
				.type="playlist",
				.id=playlistId
			};
		}
		throw SoundHoleError(SoundHoleError::Code::PARSE_FAILED, "invalid youtube media URL "+url);
	}

	String YoutubeMediaProvider::createURI(String type, String id) const {
		if(id.empty()) {
			throw std::runtime_error("Cannot create URI with empty id");
		} else if(type.empty()) {
			throw std::runtime_error("Cannot create URI with empty type");
		}
		return name()+':'+type+':'+id;
	}



	#pragma mark Login

	Promise<bool> YoutubeMediaProvider::login() {
		return youtube->login().map([=](bool loggedIn) -> bool {
			if(loggedIn) {
				storeIdentity(std::nullopt);
				setIdentityNeedsRefresh();
				getIdentity();
			}
			return loggedIn;
		});
	}

	void YoutubeMediaProvider::logout() {
		youtube->logout();
		storeIdentity(std::nullopt);
	}

	bool YoutubeMediaProvider::isLoggedIn() const {
		return youtube->isLoggedIn();
	}



	#pragma mark Current User

	Promise<ArrayList<String>> YoutubeMediaProvider::getCurrentUserURIs() {
		return getIdentity().map([=](Optional<YoutubeMediaProviderIdentity> identity) -> ArrayList<String> {
			if(!identity) {
				return ArrayList<String>{};
			}
			return identity->channels.map([&](auto& channel) -> String {
				return createURI("channel", channel.id);
			});
		});
	}

	String YoutubeMediaProvider::getIdentityFilePath() const {
		String sessionPersistKey = youtube->getAuth()->getOptions().sessionPersistKey;
		if(sessionPersistKey.empty()) {
			return String();
		}
		return utils::getCacheDirectoryPath()+"/"+name()+"_identity_"+sessionPersistKey+".json";
	}

	Promise<Optional<YoutubeMediaProviderIdentity>> YoutubeMediaProvider::fetchIdentity() {
		if(!isLoggedIn()) {
			return Promise<Optional<YoutubeMediaProviderIdentity>>::resolve(std::nullopt);
		}
		return youtube->getMyChannels().map([=](YoutubePage<YoutubeChannel> channelPage) -> Optional<YoutubeMediaProviderIdentity> {
			return maybe(YoutubeMediaProviderIdentity{
				.channels=channelPage.items
			});
		});
	}

	Promise<String> YoutubeMediaProvider::getLibraryPlaylistID() {
		return getIdentity().then([=](auto identity) {
			if(!identity) {
				return Promise<String>::resolve(String());
			}
			if(!identity->libraryPlaylistId.empty()) {
				return Promise<String>::resolve(identity->libraryPlaylistId);
			}
			return fetchLibraryPlaylistID().then([=](auto playlistId) {
				if(playlistId.empty()) {
					return Promise<String>::resolve(playlistId);
				}
				return getIdentity().then([=](auto identity) {
					if(!identity) {
						return String();
					}
					identity->libraryPlaylistId = playlistId;
					this->storeIdentity(identity.value());
					return playlistId;
				});
			});
		});
	}

	Promise<String> YoutubeMediaProvider::fetchLibraryPlaylistID(String pageToken) {
		return youtube->getMyPlaylists({
			.maxResults=50,
			.pageToken=pageToken
		}).except([=](Error& error) {
			auto retryAfter = error.getDetail("retryAfter").maybeAs<double>();
			if(!retryAfter.hasValue()) {
				std::rethrow_exception(std::current_exception());
			}
			auto delayTime = std::chrono::milliseconds((long long)(retryAfter.value() * 1000));
			return Promise<YoutubePage<YoutubePlaylist>>::delayed(delayTime, [=]() {
				return youtube->getMyPlaylists({
					.maxResults=50,
					.pageToken=pageToken
				});
			});
		}).then([=](auto page) {
			auto playlist = page.items.firstWhere([=](auto p) {return (p.snippet.title == this->libraryPlaylistName);});
			if(playlist.hasValue()) {
				return Promise<String>::resolve(playlist->id);
			}
			if(!page.nextPageToken.empty()) {
				return fetchLibraryPlaylistID(page.nextPageToken);
			}
			// create playlist
			return youtube->createPlaylist(this->libraryPlaylistName, {
				.privacyStatus = YoutubePrivacyStatus::PRIVATE,
				.description = this->libraryPlaylistDescription
			}).map([=](auto playlist) {
				return playlist.id;
			});
		});
	}



	#pragma mark Search

	Promise<YoutubePage<$<MediaItem>>> YoutubeMediaProvider::search(String query, SearchOptions options) {
		return youtube->search(query, options).map([=](YoutubePage<YoutubeSearchResult> searchResults) -> YoutubePage<$<MediaItem>> {
			return searchResults.map([&](auto& item) -> $<MediaItem> {
				return createMediaItem(item);
			});
		});
	}



	#pragma mark Data transforming

	Track::Data YoutubeMediaProvider::createTrackData(YoutubeVideo video) {
		return Track::Data{{
			.partial=true,
			.type="track",
			.name=video.snippet.title,
			.uri=createURI("video", video.id),
			.images=video.snippet.thumbnails.map([&](auto image) -> MediaItem::Image {
				return createImage(image);
			})
			},
			.albumName="",
			.albumURI="",
			.artists=ArrayList<$<Artist>>{
				this->artist(Artist::Data{{
					.partial=true,
					.type="artist",
					.name=video.snippet.channelTitle,
					.uri=createURI("channel", video.snippet.channelId),
					.images=std::nullopt
					},
					.description=std::nullopt
				})
			},
			.tags=video.snippet.tags,
			.discNumber=std::nullopt,
			.trackNumber=std::nullopt,
			.duration=std::nullopt,
			.audioSources=std::nullopt,
			.playable=true
		};
	}

	Track::Data YoutubeMediaProvider::createTrackData(YoutubeVideoInfo video) {
		String title = video.playerResponse.videoDetails.title;
		String videoId = video.playerResponse.videoDetails.videoId;
		if(videoId.empty()) {
			throw SoundHoleError(SoundHoleError::Code::PARSE_FAILED, "ytdl response is missing crucial video information");
		}
		return Track::Data{{
			.partial=false,
			.type="track",
			.name=title,
			.uri=createURI("video", videoId),
			.images=([&]() -> Optional<ArrayList<MediaItem::Image>> {
				auto images = video.playerResponse.videoDetails.thumbnail.thumbnails.map([&](auto image) -> MediaItem::Image {
					auto dimensions = MediaItem::Image::Dimensions{
						.width=image.width,
						.height=image.height
					};
					return MediaItem::Image{
						.url=image.url,
						.size=dimensions.toSize(),
						.dimensions=dimensions
					};
				});
				if(!video.thumbnailURL.empty()) {
					images.pushBack(MediaItem::Image{
						.url=video.thumbnailURL,
						.size=MediaItem::Image::Size::SMALL,
						.dimensions=std::nullopt
					});
				}
				return images;
			})(),
			},
			.albumName="",
			.albumURI="",
			.artists=([&]() {
				ArrayList<$<Artist>> artists;
				if(video.author && !video.author->id.empty()) {
					artists.pushBack(this->artist(Artist::Data{{
						.partial=true,
						.type="artist",
						.name=video.author->name,
						.uri=createURI("channel", video.playerResponse.videoDetails.channelId),
						.images=([&]() -> Optional<ArrayList<MediaItem::Image>> {
							if(video.author->avatar.empty()) {
								return std::nullopt;
							}
							return ArrayList<MediaItem::Image>{
								MediaItem::Image{
									.url=video.author->avatar,
									.size=MediaItem::Image::Size::SMALL,
									.dimensions=std::nullopt
								}
							};
						})()
						},
						.description=std::nullopt
					}));
				} else if(!video.playerResponse.videoDetails.channelId.empty()) {
					artists.pushBack(this->artist(Artist::Data{{
						.partial=true,
						.type="artist",
						.name=video.playerResponse.videoDetails.author,
						.uri=createURI("channel", video.playerResponse.videoDetails.channelId),
						.images=std::nullopt
						},
						.description=std::nullopt
					}));
				}
				if(video.media) {
					String artistURI;
					if(!video.media->artistURL.empty()) {
						try {
							auto uriParts = parseURL(video.media->artistURL);
							if(uriParts.type != "channel") {
								throw SoundHoleError(SoundHoleError::Code::PARSE_FAILED, "Youtube artistURL is not a valid channel");
							}
							artistURI = createURI(uriParts.type, uriParts.id);
						} catch(SoundHoleError&) {
							// do nothing
						}
					}
					if(artistURI.empty() || !artists.containsWhere([&](auto& cmpArtist) { return (artistURI == cmpArtist->uri()); })) {
						artists.pushBack(this->artist(Artist::Data{{
							.partial=true,
							.type="artist",
							.name=video.playerResponse.videoDetails.author,
							.uri=createURI("channel", video.playerResponse.videoDetails.channelId),
							.images=std::nullopt
							},
							.description=std::nullopt
						}));
					}
				}
				return artists;
			})(),
			.tags=video.playerResponse.videoDetails.keywords,
			.discNumber=std::nullopt,
			.trackNumber=std::nullopt,
			.duration=maybeTry([&]() {
				return video.playerResponse.videoDetails.lengthSeconds.toArithmeticValue<double>();
			}),
			.audioSources=video.formats.where([&](auto& format) {
				return format.audioBitrate.has_value();
			}).map([&](auto& format) -> Track::AudioSource {
				return Track::AudioSource{
					.url=format.url,
					.encoding=format.audioCodec,
					.bitrate=(double)format.audioBitrate.value(),
					.videoBitrate=([&]() -> Optional<double> {
						auto bitrateParts = format.bitrate.split('-');
						if(bitrateParts.size() == 0) {
							return std::nullopt;
						}
						return maybeTry([&]{ return bitrateParts.back().template toArithmeticValue<double>(); });
					})()
				};
			}),
			.playable=true
		};
	}

	Artist::Data YoutubeMediaProvider::createArtistData(YoutubeChannel channel) {
		return Artist::Data{{
			.partial=false,
			.type="artist",
			.name=channel.snippet.title,
			.uri=createURI("channel", channel.id),
			.images=channel.snippet.thumbnails.map([&](auto image) -> MediaItem::Image {
				return createImage(image);
			})
			},
			.description=channel.snippet.description
		};
	}

	Playlist::Data YoutubeMediaProvider::createPlaylistData(YoutubePlaylist playlist) {
		return Playlist::Data{{{
			.partial = false,
			.type = "playlist",
			.name = playlist.snippet.title,
			.uri = createURI("playlist", playlist.id),
			.images = playlist.snippet.thumbnails.map([&](auto image) -> MediaItem::Image {
				return createImage(image);
			})
			},
			.versionId = playlist.etag,
			.itemCount = std::nullopt,
			.items = {}
			},
			.owner = this->userAccount(UserAccount::Data{
				.partial = true,
				.type = "user",
				.name = playlist.snippet.channelTitle,
				.uri = createURI("channel", playlist.snippet.channelId)
			}),
			.privacy = (
				(playlist.status.privacyStatus == YoutubePrivacyStatus::PRIVATE) ?
					  maybe(Playlist::Privacy::PRIVATE)
				: (playlist.status.privacyStatus == YoutubePrivacyStatus::PUBLIC) ?
					  maybe(Playlist::Privacy::PUBLIC)
				: (playlist.status.privacyStatus == YoutubePrivacyStatus::UNLISTED) ?
					  maybe(Playlist::Privacy::UNLISTED)
				: std::nullopt)
		};
	}

	PlaylistItem::Data YoutubeMediaProvider::createPlaylistItemData(YoutubePlaylistItem playlistItem) {
		return PlaylistItem::Data{{
			.track=this->track(Track::Data{{
				.partial = true,
				.type = "track",
				.name = playlistItem.snippet.title,
				.uri = createURI("video", playlistItem.snippet.resourceId.videoId),
				.images = playlistItem.snippet.thumbnails.map([&](auto image) -> MediaItem::Image {
					return createImage(image);
				})
				},
				.albumName = "",
				.albumURI = "",
				.artists=ArrayList<$<Artist>>{
					this->artist(Artist::Data{{
						.partial = true,
						.type = "artist",
						.name = playlistItem.snippet.channelTitle,
						.uri = createURI("channel", playlistItem.snippet.channelId),
						.images = std::nullopt
						},
						.description = std::nullopt
					})
				},
				.tags = std::nullopt,
				.discNumber = std::nullopt,
				.trackNumber = std::nullopt,
				.duration = std::nullopt,
				.audioSources = std::nullopt,
				.playable = true
			}),
			},
			.uniqueId = playlistItem.id,
			.addedAt = playlistItem.snippet.publishedAt,
			.addedBy = this->userAccount(UserAccount::Data{
				.partial = true,
				.type = "user",
				.name = playlistItem.snippet.channelTitle,
				.uri = createURI("channel", playlistItem.snippet.channelId),
				.images = std::nullopt
			})
		};
	}

	UserAccount::Data YoutubeMediaProvider::createUserData(YoutubeChannel channel) {
		return UserAccount::Data{
			.partial = false,
			.type = "channel",
			.name = channel.snippet.title,
			.uri = createURI("channel", channel.id),
			.images = channel.snippet.thumbnails.map([&](auto image) -> MediaItem::Image {
				return createImage(image);
			})
		};
	}

	MediaItem::Image YoutubeMediaProvider::createImage(YoutubeImage image) {
		return MediaItem::Image{
			.url=image.url,
			.size=([&]() {
				switch (image.size) {
					case YoutubeImage::Size::DEFAULT:
						return MediaItem::Image::Size::TINY;
					case YoutubeImage::Size::MEDIUM:
						return MediaItem::Image::Size::SMALL;
					case YoutubeImage::Size::HIGH:
					case YoutubeImage::Size::STANDARD:
						return MediaItem::Image::Size::MEDIUM;
					case YoutubeImage::Size::MAXRES:
						return MediaItem::Image::Size::LARGE;
				}
			})(),
			.dimensions=([&]() -> Optional<MediaItem::Image::Dimensions> {
				if(!image.dimensions) {
					return std::nullopt;
				}
				return MediaItem::Image::Dimensions{
					.width=image.dimensions->width,
					.height=image.dimensions->height
				};
			})()
		};
	}

	$<MediaItem> YoutubeMediaProvider::createMediaItem(YoutubeSearchResult searchResult) {
		auto images = searchResult.snippet.thumbnails.map([&](auto& image) -> MediaItem::Image {
			return createImage(image);
		});
		if(searchResult.id.kind == "youtube#video") {
			return this->track(Track::Data{{
				.partial=true,
				.type="track",
				.name=searchResult.snippet.title,
				.uri=createURI("video", searchResult.id.videoId.value()),
				.images=images
				},
				.albumName="",
				.albumURI="",
				.artists=ArrayList<$<Artist>>{
					this->artist(Artist::Data{{
						.partial=true,
						.type="artist",
						.name=searchResult.snippet.channelTitle,
						.uri=createURI("channel", searchResult.snippet.channelId),
						.images=std::nullopt
						},
						.description=std::nullopt
					})
				},
				.tags=std::nullopt,
				.discNumber=std::nullopt,
				.trackNumber=std::nullopt,
				.duration=std::nullopt,
				.audioSources=std::nullopt,
				.playable=true
			});
		} else if(searchResult.id.kind == "youtube#channel") {
			return this->artist(Artist::Data{{
				.partial=true,
				.type="artist",
				.name=searchResult.snippet.title,
				.uri=createURI("channel", searchResult.id.channelId.value()),
				.images=images
				},
				.description=searchResult.snippet.description
			});
		} else if(searchResult.id.kind == "youtube#playlist") {
			return this->playlist(Playlist::Data{{{
				.partial=true,
				.type="playlist",
				.name=searchResult.snippet.title,
				.uri=createURI("playlist", searchResult.id.playlistId.value()),
				.images=images
				},
				.versionId=searchResult.etag,
				.itemCount=std::nullopt,
				.items={}
				},
				.owner=this->userAccount(UserAccount::Data{
					.partial = true,
					.type = "user",
					.name = searchResult.snippet.channelTitle,
					.uri = createURI("channel", searchResult.snippet.channelId),
					.images = std::nullopt
				}),
				.privacy=std::nullopt
			});
		}
		throw std::logic_error("Invalid youtube item kind "+searchResult.id.kind);
	}



	#pragma mark Media Item Fetching

	Promise<Track::Data> YoutubeMediaProvider::getTrackData(String uri) {
		auto uriParts = parseURI(uri);
		if(uriParts.type != "video") {
			return Promise<Track::Data>::reject(std::invalid_argument(uri+" is not a video URI"));
		}
		return youtube->getVideoInfo(uriParts.id).map([=](auto track) -> Track::Data {
			return createTrackData(track);
		});
	}

	Promise<Artist::Data> YoutubeMediaProvider::getArtistData(String uri) {
		auto uriParts = parseURI(uri);
		Youtube::GetChannelsOptions options;
		if(uriParts.type == "channel") {
			options.ids = { uriParts.id };
		} else if(uriParts.type == "user") {
			options.forUsername = uriParts.id;
		} else {
			return Promise<Artist::Data>::reject(std::invalid_argument(uri+" is not a channel or user URI"));
		}
		return youtube->getChannels(options).map([=](auto page) -> Artist::Data {
			if(page.items.size() == 0) {
				throw YoutubeError(YoutubeError::Code::NOT_FOUND, "Could not find channel with for uri "+uri);
			}
			return createArtistData(page.items.front());
		});
	}

	Promise<Album::Data> YoutubeMediaProvider::getAlbumData(String uri) {
		return Promise<Album::Data>::reject(std::logic_error("Youtube does not support albums"));
	}

	Promise<Playlist::Data> YoutubeMediaProvider::getPlaylistData(String uri) {
		auto uriParts = parseURI(uri);
		if(uriParts.type != "playlist") {
			throw std::invalid_argument(uri+" is not a playlist URI");
		}
		return youtube->getPlaylist(uriParts.id).map([=](auto playlist) -> Playlist::Data {
			return createPlaylistData(playlist);
		});
	}

	Promise<UserAccount::Data> YoutubeMediaProvider::getUserData(String uri) {
		auto uriParts = parseURI(uri);
		Youtube::GetChannelsOptions options;
		if(uriParts.type == "channel") {
			options.ids = { uriParts.id };
		} else if(uriParts.type == "user") {
			options.forUsername = uriParts.id;
		} else {
			return Promise<UserAccount::Data>::reject(std::invalid_argument(uri+" is not a channel or user URI"));
		}
		return youtube->getChannels(options).map([=](auto page) -> UserAccount::Data {
			if(page.items.size() == 0) {
				throw YoutubeError(YoutubeError::Code::NOT_FOUND, "Could not find channel with for uri "+uri);
			}
			return createUserData(page.items.front());
		});
	}



	Promise<ArrayList<$<Track>>> YoutubeMediaProvider::getArtistTopTracks(String artistURI) {
		auto uriParts = parseURI(artistURI);
		if(uriParts.type != "channel") {
			throw std::invalid_argument(artistURI+" is not a channel URI");
		}
		return youtube->search("", {
			.types={ Youtube::MediaType::VIDEO },
			.maxResults=10,
			.channelId=uriParts.id,
			.order=Youtube::SearchOrder::VIEW_COUNT
		}).map([=](auto page) -> ArrayList<$<Track>> {
			return page.items.map([&](auto searchResult) -> $<Track> {
				if(searchResult.kind != "youtube#video") {
					throw std::invalid_argument("Youtube::search returned unexpected item kind "+searchResult.kind);
				}
				return std::static_pointer_cast<Track>(createMediaItem(searchResult));
			});
		});
	}

	YoutubeMediaProvider::ArtistAlbumsGenerator YoutubeMediaProvider::getArtistAlbums(String artistURI) {
		using YieldResult = typename ArtistAlbumsGenerator::YieldResult;
		return ArtistAlbumsGenerator([=]() {
			return Promise<YieldResult>::reject(std::logic_error("Youtube does not support albums"));
		});
	}



	YoutubeMediaProvider::UserPlaylistsGenerator YoutubeMediaProvider::getUserPlaylists(String userURI) {
		auto uriParts = parseURI(userURI);
		if(uriParts.type != "channel") {
			throw std::invalid_argument(userURI+" is not a channel URI");
		}
		String channelId = uriParts.id;
		struct SharedData {
			String pageToken;
		};
		auto sharedData = fgl::new$<SharedData>();
		using YieldResult = typename UserPlaylistsGenerator::YieldResult;
		return UserPlaylistsGenerator([=]() {
			return youtube->getChannelPlaylists(channelId, {
				.maxResults=20,
				.pageToken=sharedData->pageToken
			}).map([=](auto page) -> YieldResult {
				auto loadBatch = LoadBatch<$<Playlist>>{
					.items=page.items.map([=](auto playlist) -> $<Playlist> {
						return this->playlist(createPlaylistData(playlist));
					}),
					.total=page.pageInfo.totalResults
				};
				sharedData->pageToken = page.nextPageToken;
				bool done = sharedData->pageToken.empty();
				return YieldResult{
					.value=loadBatch,
					.done=done
				};
			});
		});
	}



	Album::MutatorDelegate* YoutubeMediaProvider::createAlbumMutatorDelegate($<Album> album) {
		throw std::logic_error("Youtube does not support albums");
	}

	Playlist::MutatorDelegate* YoutubeMediaProvider::createPlaylistMutatorDelegate($<Playlist> playlist) {
		return new YoutubePlaylistMutatorDelegate(playlist);
	}



	#pragma mark User Library

	bool YoutubeMediaProvider::hasLibrary() const {
		return false;
	}

	bool YoutubeMediaProvider::handlesUsersAsArtists() const {
		return true;
	}

	YoutubeMediaProvider::LibraryItemGenerator YoutubeMediaProvider::generateLibrary(GenerateLibraryOptions options) {
		using YieldResult = typename LibraryItemGenerator::YieldResult;
		return LibraryItemGenerator([=]() {
			// TODO implement YoutubeMediaProvider::generateLibrary
			return Promise<YieldResult>::resolve(YieldResult{
				.done=true
			});
		});
	}



	#pragma mark Following / Unfollowing

	bool YoutubeMediaProvider::canFollowArtists() const {
		return false;
	}

	Promise<void> YoutubeMediaProvider::followArtist(String artistURI) {
		return Promise<void>::reject(std::runtime_error("not implemented"));
	}

	Promise<void> YoutubeMediaProvider::unfollowArtist(String artistURI) {
		return Promise<void>::reject(std::runtime_error("not implemented"));
	}

	bool YoutubeMediaProvider::canFollowUsers() const {
		return false;
	}

	Promise<void> YoutubeMediaProvider::followUser(String userURI) {
		return Promise<void>::reject(std::runtime_error("not implemented"));
	}

	Promise<void> YoutubeMediaProvider::unfollowUser(String userURI) {
		return Promise<void>::reject(std::runtime_error("not implemented"));
	}

	Promise<void> YoutubeMediaProvider::saveTrack(String trackURI) {
		auto uriParts = parseURI(trackURI);
		return getLibraryPlaylistID().then([=](auto libraryPlaylistId) {
			if(libraryPlaylistId.empty()) {
				throw std::runtime_error("No library playlist available");
			}
			// check if item is already saved
			return youtube->getPlaylistItems(libraryPlaylistId, {
				.maxResults = 1,
				.videoId = uriParts.id
			}).then([=](auto page) {
				if(page.items.size() > 0) {
					return Promise<void>::resolve();
				}
				// save item
				return youtube->insertPlaylistItem(libraryPlaylistId, uriParts.id, {
					.position = 0
				}).toVoid();
			});
		});
	}

	Promise<void> YoutubeMediaProvider::unsaveTrack(String trackURI) {
		auto uriParts = parseURI(trackURI);
		return getLibraryPlaylistID().then([=](auto libraryPlaylistId) {
			if(libraryPlaylistId.empty()) {
				throw std::runtime_error("No library playlist available");
			}
			// check if item is already saved
			return youtube->getPlaylistItems(libraryPlaylistId, {
				.maxResults = 50,
				.videoId = uriParts.id
			}).then([=](auto page) {
				if(page.items.size() == 0) {
					return Promise<void>::resolve();
				}
				// delete all items
				return Promise<void>::all(page.items.map([=](auto& pageItem) {
					return youtube->deletePlaylistItem(pageItem.id);
				}));
			});
		});
	}

	Promise<void> YoutubeMediaProvider::saveAlbum(String albumURI) {
		return Promise<void>::reject(std::runtime_error("Youtube does not have albums"));
	}

	Promise<void> YoutubeMediaProvider::unsaveAlbum(String albumURI) {
		return Promise<void>::reject(std::runtime_error("Youtube does not have albums"));
	}

	bool YoutubeMediaProvider::canSavePlaylists() const {
		return false;
	}

	Promise<void> YoutubeMediaProvider::savePlaylist(String playlistURI) {
		return Promise<void>::reject(std::runtime_error("Playlist saving is not implemented for youtube"));
	}

	Promise<void> YoutubeMediaProvider::unsavePlaylist(String playlistURI) {
		return Promise<void>::reject(std::runtime_error("Playlist saving is not implemented for youtube"));
	}



	#pragma mark Playlists

	bool YoutubeMediaProvider::canCreatePlaylists() const {
		return true;
	}

	ArrayList<Playlist::Privacy> YoutubeMediaProvider::supportedPlaylistPrivacies() const {
		return { Playlist::Privacy::PRIVATE, Playlist::Privacy::UNLISTED, Playlist::Privacy::PUBLIC };
	}

	Promise<$<Playlist>> YoutubeMediaProvider::createPlaylist(String name, CreatePlaylistOptions options) {
		YoutubePrivacyStatus::Type privacyStatus;
		switch(options.privacy) {
			case Playlist::Privacy::PRIVATE:
				privacyStatus = YoutubePrivacyStatus::PRIVATE;
				break;
			case Playlist::Privacy::UNLISTED:
				privacyStatus = YoutubePrivacyStatus::UNLISTED;
				break;
			case Playlist::Privacy::PUBLIC:
				privacyStatus = YoutubePrivacyStatus::PUBLIC;
				break;
		}
		return youtube->createPlaylist(name, {
			.privacyStatus = privacyStatus
		}).map([=](YoutubePlaylist playlist) -> $<Playlist> {
			return this->playlist(createPlaylistData(playlist));
		});
	}

	Promise<bool> YoutubeMediaProvider::isPlaylistEditable($<Playlist> playlist) {
		// TODO check if playlist is editable
		return Promise<bool>::resolve(false);
	}

	Promise<void> YoutubeMediaProvider::deletePlaylist(String playlistURI) {
		auto uriParts = parseURI(playlistURI);
		return youtube->deletePlaylist(uriParts.id);
	}



	#pragma mark Player

	YoutubePlaybackProvider* YoutubeMediaProvider::player() {
		return _player;
	}

	const YoutubePlaybackProvider* YoutubeMediaProvider::player() const {
		return _player;
	}



	#pragma mark YoutubeMediaProviderIdentity

	YoutubeMediaProviderIdentity YoutubeMediaProviderIdentity::fromJson(const Json& json) {
		auto channelsJson = json["channels"];
		if(!channelsJson.is_array()) {
			throw std::invalid_argument("Invalid youtube provider identity json");
		}
		ArrayList<YoutubeChannel> channels;
		channels.reserve(channelsJson.array_items().size());
		for(auto& channelJson : channelsJson.array_items()) {
			channels.pushBack(YoutubeChannel::fromJson(channelJson));
		}
		return YoutubeMediaProviderIdentity{
			.channels = channels,
			.libraryPlaylistId = json["libraryPlaylistId"].string_value()
		};
	}

	Json YoutubeMediaProviderIdentity::toJson() const {
		return Json::object{
			{ "channels", channels.map([](auto& channel) -> Json {
				return channel.toJson();
			}) },
			{ "libraryPlaylistId", (std::string)libraryPlaylistId }
		};
	}
}

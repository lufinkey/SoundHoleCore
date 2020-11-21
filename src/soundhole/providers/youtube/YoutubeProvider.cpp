//
//  YoutubeProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/23/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "YoutubeProvider.hpp"
#include "api/YoutubeError.hpp"
#include "mutators/YoutubePlaylistMutatorDelegate.hpp"
#include <soundhole/utils/SoundHoleError.hpp>
#include <soundhole/utils/Utils.hpp>

namespace sh {
	YoutubeProvider::YoutubeProvider(Options options)
	: youtube(new Youtube(options)), _player(new YoutubePlaybackProvider(this)) {
		//
	}

	YoutubeProvider::~YoutubeProvider() {
		delete _player;
		delete youtube;
	}
	
	String YoutubeProvider::name() const {
		return "youtube";
	}
	
	String YoutubeProvider::displayName() const {
		return "YouTube";
	}




	Youtube* YoutubeProvider::api() {
		return youtube;
	}

	const Youtube* YoutubeProvider::api() const {
		return youtube;
	}



	#pragma mark URI/URL parsing

	YoutubeProvider::URI YoutubeProvider::parseURI(String uri) const {
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

	YoutubeProvider::URI YoutubeProvider::parseURL(String url) const {
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

	String YoutubeProvider::createURI(String type, String id) const {
		if(id.empty()) {
			throw std::runtime_error("Cannot create URI with empty id");
		} else if(type.empty()) {
			throw std::runtime_error("Cannot create URI with empty type");
		}
		return name()+':'+type+':'+id;
	}



	#pragma mark Login

	Promise<bool> YoutubeProvider::login() {
		return youtube->login().map<bool>([=](bool loggedIn) {
			getCurrentUserYoutubeChannels();
			return loggedIn;
		});
	}

	void YoutubeProvider::logout() {
		youtube->logout();
		setCurrentUserYoutubeChannels({});
	}

	bool YoutubeProvider::isLoggedIn() const {
		return youtube->isLoggedIn();
	}



	#pragma mark Current User

	Promise<ArrayList<String>> YoutubeProvider::getCurrentUserIds() {
		return getCurrentUserYoutubeChannels().map<ArrayList<String>>([=](ArrayList<YoutubeChannel> channels) -> ArrayList<String> {
			return channels.map<String>([](auto& channel) {
				return channel.id;
			});
		});
	}

	String YoutubeProvider::getCachedCurrentUserYoutubeChannelsPath() const {
		String sessionPersistKey = youtube->getAuth()->getOptions().sessionPersistKey;
		if(sessionPersistKey.empty()) {
			return String();
		}
		return utils::getCacheDirectoryPath()+"/"+name()+"_channels_"+sessionPersistKey+".json";
	}

	Promise<ArrayList<YoutubeChannel>> YoutubeProvider::getCurrentUserYoutubeChannels() {
		if(!isLoggedIn()) {
			setCurrentUserYoutubeChannels({});
			return Promise<ArrayList<YoutubeChannel>>::resolve({});
		}
		if(_currentUserChannelsPromise) {
			return _currentUserChannelsPromise.value();
		}
		if(!_currentUserChannels.empty() && !_currentUserNeedsRefresh) {
			return Promise<ArrayList<YoutubeChannel>>::resolve(_currentUserChannels);
		}
		auto promise = youtube->getMyChannels().map<ArrayList<YoutubeChannel>>([=](YoutubePage<YoutubeChannel> channelPage) -> ArrayList<YoutubeChannel> {
			_currentUserChannelsPromise = std::nullopt;
			if(!isLoggedIn()) {
				setCurrentUserYoutubeChannels({});
				return {};
			}
			setCurrentUserYoutubeChannels(channelPage.items);
			return channelPage.items;
		}).except([=](std::exception_ptr error) -> ArrayList<YoutubeChannel> {
			_currentUserChannelsPromise = std::nullopt;
			// if current user is loaded, return it
			if(!_currentUserChannels.empty()) {
				return _currentUserChannels;
			}
			// try to load user from filesystem
			auto userFilePath = getCachedCurrentUserYoutubeChannelsPath();
			if(!userFilePath.empty()) {
				try {
					String userString = fs::readFile(userFilePath);
					std::string jsonError;
					Json userJson = Json::parse(userString, jsonError);
					if(userJson.is_null()) {
						throw std::runtime_error("failed to parse user json: "+jsonError);
					}
					auto channels = ArrayList<Json>(userJson["channels"].array_items()).map<YoutubeChannel>([=](auto& json) {
						return YoutubeChannel::fromJson(json);
					});
					_currentUserChannels = channels;
					_currentUserNeedsRefresh = true;
					return channels;
				} catch(...) {
					// ignore error
				}
			}
			// rethrow error
			std::rethrow_exception(error);
		});
		_currentUserChannelsPromise = promise;
		if(_currentUserChannelsPromise && _currentUserChannelsPromise->isComplete()) {
			_currentUserChannelsPromise = std::nullopt;
		}
		return promise;
	}

	void YoutubeProvider::setCurrentUserYoutubeChannels(ArrayList<YoutubeChannel> channels) {
		auto userFilePath = getCachedCurrentUserYoutubeChannelsPath();
		if(!channels.empty()) {
			_currentUserChannels = channels;
			_currentUserNeedsRefresh = false;
			auto json = Json(channels.map<Json>([](auto& channel) {
				return channel.toJson();
			}));
			if(!userFilePath.empty()) {
				fs::writeFile(userFilePath, json.dump());
			}
		} else {
			_currentUserChannels = {};
			_currentUserChannelsPromise = std::nullopt;
			_currentUserNeedsRefresh = true;
			if(!userFilePath.empty() && fs::exists(userFilePath)) {
				fs::remove(userFilePath);
			}
		}
	}



	#pragma mark Search

	Promise<YoutubePage<$<MediaItem>>> YoutubeProvider::search(String query, SearchOptions options) {
		return youtube->search(query, options).map<YoutubePage<$<MediaItem>>>([=](YoutubePage<YoutubeSearchResult> searchResults) {
			return searchResults.template map<$<MediaItem>>([&](auto& item) {
				return createMediaItem(item);
			});
		});
	}



	#pragma mark Data transforming

	Track::Data YoutubeProvider::createTrackData(YoutubeVideo video) {
		return Track::Data{{
			.partial=true,
			.type="track",
			.name=video.snippet.title,
			.uri=createURI("video", video.id),
			.images=video.snippet.thumbnails.map<MediaItem::Image>([&](auto image) {
				return createImage(image);
			})
			},
			.albumName="",
			.albumURI="",
			.artists=ArrayList<$<Artist>>{
				Artist::new$(this, Artist::Data{{
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

	Track::Data YoutubeProvider::createTrackData(YoutubeVideoInfo video) {
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
				auto images = video.playerResponse.videoDetails.thumbnail.thumbnails.map<MediaItem::Image>([&](auto image) {
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
					artists.pushBack(Artist::new$(this, Artist::Data{{
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
					artists.pushBack(Artist::new$(this, Artist::Data{{
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
						artists.pushBack(Artist::new$(this, Artist::Data{{
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
			}).map<Track::AudioSource>([&](auto& format) {
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

	Artist::Data YoutubeProvider::createArtistData(YoutubeChannel channel) {
		return Artist::Data{{
			.partial=false,
			.type="artist",
			.name=channel.snippet.title,
			.uri=createURI("channel", channel.id),
			.images=channel.snippet.thumbnails.map<MediaItem::Image>([&](auto image) {
				return createImage(image);
			})
			},
			.description=channel.snippet.description
		};
	}

	Playlist::Data YoutubeProvider::createPlaylistData(YoutubePlaylist playlist) {
		return Playlist::Data{{{
			.partial=false,
			.type="playlist",
			.name=playlist.snippet.title,
			.uri=createURI("playlist", playlist.id),
			.images=playlist.snippet.thumbnails.map<MediaItem::Image>([&](auto image) {
				return createImage(image);
			})
			},
			.versionId=playlist.etag,
			.tracks=std::nullopt,
			},
			.owner=UserAccount::new$(this, UserAccount::Data{{
				.partial=true,
				.type="user",
				.name=playlist.snippet.channelTitle,
				.uri=createURI("channel", playlist.snippet.channelId)
				},
				.id=playlist.snippet.channelId,
				.displayName=playlist.snippet.channelTitle
			}),
			.privacy=(
				(playlist.status.privacyStatus == YoutubePrivacyStatus::PRIVATE) ?
					  Playlist::Privacy::PRIVATE
				: (playlist.status.privacyStatus == YoutubePrivacyStatus::PUBLIC) ?
					  Playlist::Privacy::PUBLIC
				: (playlist.status.privacyStatus == YoutubePrivacyStatus::UNLISTED) ?
					  Playlist::Privacy::UNLISTED
				: Playlist::Privacy::UNKNOWN)
		};
	}

	PlaylistItem::Data YoutubeProvider::createPlaylistItemData(YoutubePlaylistItem playlistItem) {
		return PlaylistItem::Data{{
			.track=Track::new$(this, Track::Data{{
				.partial=true,
				.type="track",
				.name=playlistItem.snippet.title,
				.uri=createURI("video", playlistItem.snippet.resourceId.videoId),
				.images=playlistItem.snippet.thumbnails.map<MediaItem::Image>([&](auto image) {
					return createImage(image);
				})
				},
				.albumName="",
				.albumURI="",
				.artists=ArrayList<$<Artist>>{
					Artist::new$(this, Artist::Data{{
						.partial=true,
						.type="artist",
						.name=playlistItem.snippet.channelTitle,
						.uri=createURI("channel", playlistItem.snippet.channelId),
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
			}),
			},
			.uniqueId=playlistItem.id,
			.addedAt=playlistItem.snippet.publishedAt,
			.addedBy=UserAccount::new$(this, UserAccount::Data{{
				.partial=true,
				.type="user",
				.name=playlistItem.snippet.channelTitle,
				.uri=createURI("channel", playlistItem.snippet.channelId),
				.images=std::nullopt
				},
				.id=playlistItem.snippet.channelId,
				.displayName=playlistItem.snippet.channelTitle
			})
		};
	}

	MediaItem::Image YoutubeProvider::createImage(YoutubeImage image) {
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

	$<MediaItem> YoutubeProvider::createMediaItem(YoutubeSearchResult searchResult) {
		auto images = searchResult.snippet.thumbnails.template map<MediaItem::Image>([&](auto& image) {
			return createImage(image);
		});
		if(searchResult.id.kind == "youtube#video") {
			return std::static_pointer_cast<MediaItem>(Track::new$(this, Track::Data{{
				.partial=true,
				.type="track",
				.name=searchResult.snippet.title,
				.uri=createURI("video", searchResult.id.videoId.value()),
				.images=images
				},
				.albumName="",
				.albumURI="",
				.artists=ArrayList<$<Artist>>{
					Artist::new$(this, Artist::Data{{
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
			}));
		} else if(searchResult.id.kind == "youtube#channel") {
			return std::static_pointer_cast<MediaItem>(Artist::new$(this, Artist::Data{{
				.partial=true,
				.type="artist",
				.name=searchResult.snippet.title,
				.uri=createURI("channel", searchResult.id.channelId.value()),
				.images=images
				},
				.description=searchResult.snippet.description
			}));
		} else if(searchResult.id.kind == "youtube#playlist") {
			return std::static_pointer_cast<MediaItem>(Playlist::new$(this, Playlist::Data{{{
				.partial=true,
				.type="playlist",
				.name=searchResult.snippet.title,
				.uri=createURI("playlist", searchResult.id.playlistId.value()),
				.images=images
				},
				.versionId=searchResult.etag,
				.tracks=std::nullopt
				},
				.owner=UserAccount::new$(this, UserAccount::Data{{
					.partial=true,
					.type="user",
					.name=searchResult.snippet.channelTitle,
					.uri=createURI("channel", searchResult.snippet.channelId),
					.images=std::nullopt
					},
					.id=searchResult.snippet.channelId,
					.displayName=searchResult.snippet.channelTitle
				}),
				.privacy=Playlist::Privacy::UNKNOWN
			}));
		}
		throw std::logic_error("Invalid youtube item kind "+searchResult.id.kind);
	}



	#pragma mark Media Item Fetching

	Promise<Track::Data> YoutubeProvider::getTrackData(String uri) {
		auto uriParts = parseURI(uri);
		if(uriParts.type != "video") {
			return Promise<Track::Data>::reject(std::invalid_argument(uri+" is not a video URI"));
		}
		return youtube->getVideoInfo(uriParts.id).map<Track::Data>([=](auto track) {
			return createTrackData(track);
		});
	}

	Promise<Artist::Data> YoutubeProvider::getArtistData(String uri) {
		auto uriParts = parseURI(uri);
		Youtube::GetChannelsOptions options;
		if(uriParts.type == "channel") {
			options.ids = { uriParts.id };
		} else if(uriParts.type == "user") {
			options.forUsername = uriParts.id;
		} else {
			return Promise<Artist::Data>::reject(std::invalid_argument(uri+" is not a channel or user URI"));
		}
		return youtube->getChannels(options).map<Artist::Data>([=](auto page) {
			if(page.items.size() == 0) {
				throw YoutubeError(YoutubeError::Code::NOT_FOUND, "Could not find channel with for uri "+uri);
			}
			return createArtistData(page.items.front());
		});
	}

	Promise<Album::Data> YoutubeProvider::getAlbumData(String uri) {
		return Promise<Album::Data>::reject(std::logic_error("Youtube does not support albums"));
	}

	Promise<Playlist::Data> YoutubeProvider::getPlaylistData(String uri) {
		auto uriParts = parseURI(uri);
		if(uriParts.type != "playlist") {
			throw std::invalid_argument(uri+" is not a playlist URI");
		}
		return youtube->getPlaylist(uriParts.id).map<Playlist::Data>([=](auto playlist) {
			return createPlaylistData(playlist);
		});
	}

	Promise<UserAccount::Data> YoutubeProvider::getUserData(String uri) {
		return Promise<UserAccount::Data>::reject(std::logic_error("This method is not implemented yet"));
	}



	Promise<ArrayList<$<Track>>> YoutubeProvider::getArtistTopTracks(String artistURI) {
		auto uriParts = parseURI(artistURI);
		if(uriParts.type != "channel") {
			throw std::invalid_argument(artistURI+" is not a channel URI");
		}
		return youtube->search("", {
			.types={ Youtube::MediaType::VIDEO },
			.maxResults=10,
			.channelId=uriParts.id,
			.order=Youtube::SearchOrder::VIEW_COUNT
		}).map<ArrayList<$<Track>>>([=](auto page) {
			return page.items.template map<$<Track>>([&](auto searchResult) {
				if(searchResult.kind != "youtube#video") {
					throw std::invalid_argument("Youtube::search returned unexpected item kind "+searchResult.kind);
				}
				return std::static_pointer_cast<Track>(createMediaItem(searchResult));
			});
		});
	}

	YoutubeProvider::ArtistAlbumsGenerator YoutubeProvider::getArtistAlbums(String artistURI) {
		using YieldResult = typename ArtistAlbumsGenerator::YieldResult;
		return ArtistAlbumsGenerator([=]() {
			return Promise<YieldResult>::reject(std::logic_error("Youtube does not support albums"));
		});
	}



	YoutubeProvider::UserPlaylistsGenerator YoutubeProvider::getUserPlaylists(String userURI) {
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
			}).map<YieldResult>([=](auto page) {
				auto loadBatch = LoadBatch<$<Playlist>>{
					.items=page.items.template map<$<Playlist>>([=](auto playlist) {
						return Playlist::new$(this, createPlaylistData(playlist));
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



	Album::MutatorDelegate* YoutubeProvider::createAlbumMutatorDelegate($<Album> album) {
		throw std::logic_error("Youtube does not support albums");
	}

	Playlist::MutatorDelegate* YoutubeProvider::createPlaylistMutatorDelegate($<Playlist> playlist) {
		return new YoutubePlaylistMutatorDelegate(playlist);
	}



	#pragma mark User Library

	bool YoutubeProvider::hasLibrary() const {
		return false;
	}

	YoutubeProvider::LibraryItemGenerator YoutubeProvider::generateLibrary(GenerateLibraryOptions options) {
		using YieldResult = typename LibraryItemGenerator::YieldResult;
		return LibraryItemGenerator([=]() {
			// TODO implement YoutubeProvider::generateLibrary
			return Promise<YieldResult>::resolve(YieldResult{
				.done=true
			});
		});
	}



	#pragma mark Playlists

	bool YoutubeProvider::canCreatePlaylists() const {
		return true;
	}

	ArrayList<Playlist::Privacy> YoutubeProvider::supportedPlaylistPrivacies() const {
		return { Playlist::Privacy::PRIVATE, Playlist::Privacy::UNLISTED, Playlist::Privacy::PUBLIC };
	}

	Promise<$<Playlist>> YoutubeProvider::createPlaylist(String name, CreatePlaylistOptions options) {
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
			case Playlist::Privacy::UNKNOWN:
				return Promise<$<Playlist>>::reject(std::logic_error("invalid playlist privacy value"));
		}
		return youtube->createPlaylist(name, {
			.privacyStatus = privacyStatus
		}).map<$<Playlist>>([=](YoutubePlaylist playlist) {
			return Playlist::new$(this, createPlaylistData(playlist));
		});
	}

	Promise<bool> YoutubeProvider::isPlaylistEditable($<Playlist> playlist) {
		// TODO check if playlist is editable
		return Promise<bool>::resolve(false);
	}



	#pragma mark Player

	YoutubePlaybackProvider* YoutubeProvider::player() {
		return _player;
	}

	const YoutubePlaybackProvider* YoutubeProvider::player() const {
		return _player;
	}
}

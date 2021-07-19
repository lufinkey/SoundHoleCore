//
//  SpotifyMediaProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SpotifyMediaProvider.hpp"
#include "mutators/SpotifyAlbumMutatorDelegate.hpp"
#include "mutators/SpotifyPlaylistMutatorDelegate.hpp"
#include <soundhole/utils/SoundHoleError.hpp>
#include <soundhole/utils/Utils.hpp>

namespace sh {
	SpotifyMediaProvider::SpotifyMediaProvider(Options options)
	: spotify(new Spotify(options)),
	_player(new SpotifyPlaybackProvider(this)) {
		//
	}

	SpotifyMediaProvider::~SpotifyMediaProvider() {
		delete _player;
		delete spotify;
	}



	String SpotifyMediaProvider::name() const {
		return NAME;
	}
	
	String SpotifyMediaProvider::displayName() const {
		return "Spotify";
	}



	Spotify* SpotifyMediaProvider::api() {
		return spotify;
	}

	const Spotify* SpotifyMediaProvider::api() const {
		return spotify;
	}



	time_t SpotifyMediaProvider::timeFromString(String time) {
		if(auto date = DateTime::fromGmtString(time, "%Y-%m-%dT%H:%M:%SZ")) {
			return date->toTimeType();
		}
		throw std::invalid_argument("Invalid date format");
	}



	#pragma mark URI parsing

	SpotifyMediaProvider::URI SpotifyMediaProvider::parseURI(String uri) {
		if(uri.empty()) {
			throw std::invalid_argument("Empty string is not a valid Spotify uri");
		}
		auto parts = uri.split(':');
		if(parts.size() < 2) {
			throw SoundHoleError(SoundHoleError::Code::PARSE_FAILED, "invalid Spotify uri "+uri);
		}
		else if(parts.size() == 2) {
			return URI{
				.provider=parts.front(),
				.type=String(),
				.id=parts.back()
			};
		}
		return URI{
			.provider=parts.front(),
			.type=*std::prev(parts.end(),2),
			.id=parts.back()
		};
	}



	#pragma mark Login

	Promise<bool> SpotifyMediaProvider::login() {
		return spotify->login().map([=](bool loggedIn) -> bool {
			if(loggedIn) {
				storeIdentity(std::nullopt);
				setIdentityNeedsRefresh();
				getIdentity();
			}
			return loggedIn;
		});
	}

	void SpotifyMediaProvider::logout() {
		spotify->logout();
		storeIdentity(std::nullopt);
	}

	bool SpotifyMediaProvider::isLoggedIn() const {
		return spotify->isLoggedIn();
	}



	#pragma mark Current User

	Promise<ArrayList<String>> SpotifyMediaProvider::getCurrentUserURIs() {
		return getIdentity().map([=](Optional<SpotifyUser> user) -> ArrayList<String> {
			if(!user) {
				return {};
			}
			return { user->uri };
		});
	}

	String SpotifyMediaProvider::getIdentityFilePath() const {
		String sessionPersistKey = spotify->getAuth()->getOptions().sessionPersistKey;
		if(sessionPersistKey.empty()) {
			return String();
		}
		return utils::getCacheDirectoryPath()+"/"+name()+"_identity_"+sessionPersistKey+".json";
	}

	Promise<Optional<SpotifyUser>> SpotifyMediaProvider::fetchIdentity() {
		if(!isLoggedIn()) {
			return Promise<Optional<SpotifyUser>>::resolve(std::nullopt);
		}
		return spotify->getMe().map([=](SpotifyUser user) -> Optional<SpotifyUser> {
			return maybe(user);
		});
	}



	#pragma mark Search

	Promise<SpotifyMediaProvider::SearchResults> SpotifyMediaProvider::search(String query, SearchOptions options) {
		Spotify::SearchOptions searchOptions = {
			.types=options.types,
			.market="from_token",
			.limit=options.limit,
			.offset=options.offset
		};
		return spotify->search(query,searchOptions).map([=](auto searchResults) -> SpotifyMediaProvider::SearchResults {
			return SearchResults{
				.tracks = searchResults.tracks ?
					maybe(searchResults.tracks->map([&](auto& track) -> $<Track> {
						return this->track(createTrackData(track, false));
					}))
					: std::nullopt,
				.albums = searchResults.albums ?
					maybe(searchResults.albums->map([&](auto& album) -> $<Album> {
						return this->album(createAlbumData(album, true));
					}))
					: std::nullopt,
				.artists = searchResults.artists ?
					maybe(searchResults.artists->map([&](auto& artist) -> $<Artist> {
						return this->artist(createArtistData(artist, false));
					}))
					: std::nullopt,
				.playlists = searchResults.playlists ?
					maybe(searchResults.playlists->map([&](auto& playlist) -> $<Playlist> {
						return this->playlist(createPlaylistData(playlist, true));
					}))
					: std::nullopt,
			};
		});
	}



	#pragma mark Data transforming

	Track::Data SpotifyMediaProvider::createTrackData(SpotifyTrack track, bool partial) {
		return Track::Data{{
			.partial = partial,
			.type = track.type,
			.name = track.name,
			.uri = track.uri,
			.images = (track.album ?
				maybe(track.album->images.map([&](SpotifyImage& image) -> MediaItem::Image {
					return createImage(std::move(image));
				}))
				: std::nullopt)
			},
			.albumName = (track.album ? track.album->name : ""),
			.albumURI = (track.album ? track.album->uri : ""),
			.artists = track.artists.map([&](SpotifyArtist& artist) -> $<Artist> {
				return this->artist(createArtistData(std::move(artist), true));
			}),
			.tags = ArrayList<String>(),
			.discNumber = track.discNumber,
			.trackNumber = track.trackNumber,
			.duration = (((double)track.durationMs)/1000.0),
			.audioSources = std::nullopt,
			.playable = track.isPlayable.hasValue() ? track.isPlayable
				: track.availableMarkets.hasValue() ? maybe(track.availableMarkets->size() > 0)
				: std::nullopt
		};
	}

	Artist::Data SpotifyMediaProvider::createArtistData(SpotifyArtist artist, bool partial) {
		return Artist::Data{{
			.partial = partial,
			.type = artist.type,
			.name = artist.name,
			.uri = artist.uri,
			.images = (artist.images ?
				maybe(artist.images->map([&](SpotifyImage& image) -> MediaItem::Image {
					return createImage(std::move(image));
				}))
				: std::nullopt)
			},
			.description = ""
		};
	}

	Album::Data SpotifyMediaProvider::createAlbumData(SpotifyAlbum album, bool partial) {
		auto artists = album.artists.map([&](SpotifyArtist& artist) -> $<Artist> {
			return this->artist(createArtistData(artist, true));
		});
		auto albumImages = album.images.map([&](SpotifyImage& image) -> MediaItem::Image {
			return createImage(std::move(image));
		});
		return Album::Data{{{
			.partial = partial,
			.type = album.type,
			.name = album.name,
			.uri = album.uri,
			.images = albumImages
			},
			.versionId = album.releaseDate,
			.itemCount = (album.tracks ? maybe(album.tracks->total) : std::nullopt),
			.items = ([&]() {
				auto items = std::map<size_t,AlbumItem::Data>();
				if(!album.tracks) {
					return items;
				}
				for(size_t i=0; i<album.tracks->items.size(); i++) {
					auto trackData = createTrackData(album.tracks->items[i], true);
					trackData.albumName = album.name;
					trackData.albumURI = album.uri;
					if(!trackData.images) {
						trackData.images = albumImages;
					}
					for(size_t i=0; i<trackData.artists.size(); i++) {
						auto cmpArtist = trackData.artists[i];
						auto artist = artists.firstWhere([&](auto artist) {
							return (artist->uri() == cmpArtist->uri() && artist->name() == cmpArtist->name());
						}, nullptr);
						if(artist) {
							trackData.artists[i] = artist;
						}
					}
					size_t index = album.tracks->offset + i;
					items.insert_or_assign(index, AlbumItem::Data{
						.track=this->track(trackData)
					});
				}
				return items;
			})(),
			},
			.artists = artists
		};
	}

	Playlist::Data SpotifyMediaProvider::createPlaylistData(SpotifyPlaylist playlist, bool partial) {
		return Playlist::Data{{{
			.partial = partial,
			.type = playlist.type,
			.name = playlist.name,
			.uri = playlist.uri,
			.images = playlist.images.map([&](SpotifyImage& image) -> MediaItem::Image {
				return createImage(std::move(image));
			})
			},
			.versionId = playlist.snapshotId,
			.itemCount = playlist.tracks.total,
			.items = ([&]() {
				auto items = std::map<size_t,PlaylistItem::Data>();
				for(size_t i=0; i<playlist.tracks.items.size(); i++) {
					auto& item = playlist.tracks.items[i];
					size_t index = playlist.tracks.offset + i;
					items.insert_or_assign(index, PlaylistItem::Data{{
						.track=this->track(createTrackData(item.track, true))
						},
						.addedAt=item.addedAt,
						.addedBy=item.addedBy ? this->userAccount(createUserAccountData(item.addedBy.value(), true)) : nullptr
					});
				}
				return items;
			})(),
			},
			.owner=this->userAccount(createUserAccountData(playlist.owner, true)),
			.privacy=(playlist.isPublic ?
				(playlist.isPublic.value() ? Playlist::Privacy::PUBLIC : Playlist::Privacy::PRIVATE)
				: Playlist::Privacy::UNKNOWN)
		};
	}

	PlaylistItem::Data SpotifyMediaProvider::createPlaylistItemData(SpotifyPlaylist::Item playlistItem) {
		return PlaylistItem::Data{{
			.track = this->track(createTrackData(playlistItem.track, true))
			},
			.addedAt = playlistItem.addedAt,
			.addedBy = playlistItem.addedBy ? this->userAccount(createUserAccountData(playlistItem.addedBy.value(), true)) : nullptr
		};
	}

	UserAccount::Data SpotifyMediaProvider::createUserAccountData(SpotifyUser user, bool partial) {
		return UserAccount::Data{
			.partial = partial,
			.type = user.type,
			.name = user.displayName.value_or(user.id),
			.uri = user.uri,
			.images = (user.images ? maybe(user.images->map([&](SpotifyImage& image) -> MediaItem::Image {
				return createImage(std::move(image));
			})) : std::nullopt)
		};
	}

	MediaItem::Image SpotifyMediaProvider::createImage(SpotifyImage image) {
		auto dimensions = MediaItem::Image::Dimensions{ image.width, image.height };
		return MediaItem::Image{
			.url=image.url,
			.size=dimensions.toSize(),
			.dimensions=dimensions
		};
	}



	#pragma mark Media Item Fetching

	Promise<Track::Data> SpotifyMediaProvider::getTrackData(String uri) {
		auto uriParts = parseURI(uri);
		return spotify->getTrack(uriParts.id,{.market="from_token"}).map([=](SpotifyTrack track) -> Track::Data {
			return createTrackData(track, false);
		});
	}

	Promise<Artist::Data> SpotifyMediaProvider::getArtistData(String uri) {
		auto uriParts = parseURI(uri);
		return spotify->getArtist(uriParts.id).map([=](SpotifyArtist artist) -> Artist::Data {
			return createArtistData(artist, false);
		});
	}

	Promise<Album::Data> SpotifyMediaProvider::getAlbumData(String uri) {
		auto uriParts = parseURI(uri);
		return spotify->getAlbum(uriParts.id,{.market="from_token"}).map([=](SpotifyAlbum album) -> Album::Data {
			return createAlbumData(album, false);
		});
	}

	Promise<Playlist::Data> SpotifyMediaProvider::getPlaylistData(String uri) {
		auto uriParts = parseURI(uri);
		return spotify->getPlaylist(uriParts.id,{.market="from_token"}).map([=](SpotifyPlaylist playlist) -> Playlist::Data {
			return createPlaylistData(playlist, false);
		});
	}

	Promise<UserAccount::Data> SpotifyMediaProvider::getUserData(String uri) {
		auto uriParts = parseURI(uri);
		return spotify->getUser(uriParts.id).map([=](SpotifyUser user) -> UserAccount::Data {
			return createUserAccountData(user, false);
		});
	}




	Promise<ArrayList<$<Track>>> SpotifyMediaProvider::getArtistTopTracks(String artistURI) {
		auto uriParts = parseURI(artistURI);
		return spotify->getArtistTopTracks(uriParts.id,"from_token").map(nullptr, [=](ArrayList<SpotifyTrack> tracks) -> ArrayList<$<Track>> {
			return tracks.map([=](auto track) -> $<Track> {
				return this->track(createTrackData(track, false));
			});
		});
	}

	SpotifyMediaProvider::ArtistAlbumsGenerator SpotifyMediaProvider::getArtistAlbums(String artistURI) {
		auto uriParts = parseURI(artistURI);
		auto offset = fgl::new$<size_t>(0);
		using YieldResult = typename ArtistAlbumsGenerator::YieldResult;
		return ArtistAlbumsGenerator([=]() {
			return spotify->getArtistAlbums(uriParts.id, {
				.country="from_token",
				.limit=20,
				.offset=*offset
			}).map([=](auto page) -> YieldResult {
				auto loadBatch = LoadBatch<$<Album>>{
					.items=page.items.template map([=](auto album) -> $<Album> {
						return this->album(createAlbumData(album, true));
					}),
					.total=page.total
				};
				*offset += page.items.size();
				bool done = (*offset >= page.total || page.next.empty());
				return YieldResult{
					.value=loadBatch,
					.done=done
				};
			});
		});
	}

	SpotifyMediaProvider::UserPlaylistsGenerator SpotifyMediaProvider::getUserPlaylists(String userURI) {
		auto uriParts = parseURI(userURI);
		auto offset = fgl::new$<size_t>(0);
		using YieldResult = typename UserPlaylistsGenerator::YieldResult;
		return UserPlaylistsGenerator([=]() {
			return spotify->getUserPlaylists(uriParts.id, {
				.limit=20,
				.offset=*offset
			}).map([=](auto page) -> YieldResult {
				auto loadBatch = LoadBatch<$<Playlist>>{
					.items=page.items.map([=](auto playlist) -> $<Playlist> {
						return this->playlist(createPlaylistData(playlist, true));
					}),
					.total=page.total
				};
				*offset += page.items.size();
				bool done = (*offset >= page.total || page.next.empty());
				return YieldResult{
					.value=loadBatch,
					.done=done
				};
			});
		});
	}




	Album::MutatorDelegate* SpotifyMediaProvider::createAlbumMutatorDelegate($<Album> album) {
		return new SpotifyAlbumMutatorDelegate(album);
	}

	Playlist::MutatorDelegate* SpotifyMediaProvider::createPlaylistMutatorDelegate($<Playlist> playlist) {
		return new SpotifyPlaylistMutatorDelegate(playlist);
	}



	#pragma mark User Library

	SpotifyMediaProvider::GenerateLibraryResumeData SpotifyMediaProvider::GenerateLibraryResumeData::fromJson(const Json& json) {
		auto mostRecentTrackSave = json["mostRecentTrackSave"];
		auto mostRecentAlbumSave = json["mostRecentAlbumSave"];
		auto syncMostRecentSave = json["syncMostRecentSave"];
		auto syncLastItemOffset = json["syncLastItemOffset"];
		auto resumeData = GenerateLibraryResumeData{
			.userId = json["userId"].string_value(),
			.mostRecentTrackSave = mostRecentTrackSave.is_number() ? maybe((time_t)mostRecentTrackSave.number_value()) : std::nullopt,
			.mostRecentAlbumSave = mostRecentAlbumSave.is_number() ? maybe((time_t)mostRecentAlbumSave.number_value()) : std::nullopt,
			.syncCurrentType = json["syncCurrentType"].string_value(),
			.syncMostRecentSave = syncMostRecentSave.is_number() ? maybe((time_t)syncMostRecentSave.number_value()) : std::nullopt,
			.syncLastItemOffset = syncLastItemOffset.is_number() ? maybe((size_t)syncLastItemOffset.number_value()) : std::nullopt,
			.syncLastItem = Item::maybeFromJson(json["syncLastItem"])
		};
		bool syncTypeValid = ArrayList<String>{"albums","playlists","tracks"}.contains(resumeData.syncCurrentType);
		if(!(resumeData.syncMostRecentSave.has_value() && resumeData.syncLastItemOffset.has_value()
			 && resumeData.syncLastItem.has_value() && syncTypeValid)) {
			if(!syncTypeValid) {
				resumeData.syncCurrentType = String();
			}
			resumeData.syncMostRecentSave = std::nullopt;
			resumeData.syncLastItemOffset = std::nullopt;
			resumeData.syncLastItem = std::nullopt;
		}
		return resumeData;
	}

	Json SpotifyMediaProvider::GenerateLibraryResumeData::toJson() const {
		return Json::object{
			{ "userId", (std::string)userId },
			{ "mostRecentTrackSave", mostRecentTrackSave ? Json((double)mostRecentTrackSave.value()) : Json() },
			{ "mostRecentAlbumSave", mostRecentAlbumSave ? Json((double)mostRecentAlbumSave.value()) : Json() },
			{ "syncCurrentType", Json(syncCurrentType) },
			{ "syncMostRecentSave", syncMostRecentSave ? Json((double)syncMostRecentSave.value()) : Json() },
			{ "syncLastItemOffset", syncLastItemOffset ? Json((double)syncLastItemOffset.value()) : Json() },
			{ "syncLastItem", syncLastItem ? syncLastItem->toJson() : Json() }
		};
	}

	String SpotifyMediaProvider::GenerateLibraryResumeData::typeFromSyncIndex(size_t index) {
		switch(index) {
			case 0:
				return "playlists";
			case 1:
				return "albums";
			case 2:
				return "tracks";
		}
		return "";
	}

	Optional<size_t> SpotifyMediaProvider::GenerateLibraryResumeData::syncIndexFromType(String type) {
		if(type == "playlists") {
			return 0;
		} else if(type == "albums") {
			return 1;
		} else if(type == "tracks") {
			return 2;
		}
		return std::nullopt;
	}

	Optional<SpotifyMediaProvider::GenerateLibraryResumeData::Item> SpotifyMediaProvider::GenerateLibraryResumeData::Item::maybeFromJson(const Json& json) {
		if(!json.is_object()) {
			return std::nullopt;
		}
		auto uri = json["uri"].string_value();
		if(uri.empty()) {
			return std::nullopt;
		}
		return Item{
			.uri = uri,
			.addedAt = json["addedAt"].string_value()
		};
	}

	Json SpotifyMediaProvider::GenerateLibraryResumeData::Item::toJson() const {
		return Json::object{
			{ "uri", Json(uri) },
			{ "addedAt", Json(addedAt) }
		};
	}

	bool SpotifyMediaProvider::hasLibrary() const {
		return true;
	}

	SpotifyMediaProvider::LibraryItemGenerator SpotifyMediaProvider::generateLibrary(GenerateLibraryOptions options) {
		auto resumeData = GenerateLibraryResumeData::fromJson(options.resumeData);
		struct SharedData {
			String userId;
			Optional<time_t> mostRecentTrackSave;
			Optional<time_t> mostRecentAlbumSave;
			Optional<GenerateLibraryResumeData::Item> syncLastItem;
			Optional<size_t> syncLastItemOffset;
			Optional<time_t> syncMostRecentSave;
			size_t type = 0;
			size_t offset = 0;
			bool resuming = false;
			
			bool attemptRestoreSync(Optional<LibraryItem> firstItem) {
				if(firstItem && syncLastItem && firstItem->mediaItem->uri() == syncLastItem->uri && firstItem->addedAt == syncLastItem->addedAt) {
					return true;
				}
				syncLastItem = std::nullopt;
				syncLastItemOffset = std::nullopt;
				syncMostRecentSave = std::nullopt;
				offset = 0;
				type = 0;
				return false;
			}
		};
		
		auto sharedData = fgl::new$<SharedData>(SharedData{
			.userId = resumeData.userId,
			.mostRecentTrackSave = resumeData.mostRecentTrackSave,
			.mostRecentAlbumSave = resumeData.mostRecentAlbumSave,
			.syncLastItem = resumeData.syncLastItem,
			.syncLastItemOffset = resumeData.syncLastItemOffset,
			.syncMostRecentSave = resumeData.syncMostRecentSave,
			.type = GenerateLibraryResumeData::syncIndexFromType(resumeData.syncCurrentType).value_or(0),
			.offset = resumeData.syncLastItemOffset.value_or(0),
			.resuming = (resumeData.syncLastItem.has_value() && resumeData.syncLastItemOffset.value_or(0) > 0)
		});
		auto createResumeData = Function<GenerateLibraryResumeData($<SharedData>)>([]($<SharedData> data) {
			return GenerateLibraryResumeData{
				.userId = data->userId,
				.mostRecentTrackSave = data->mostRecentTrackSave,
				.mostRecentAlbumSave = data->mostRecentAlbumSave,
				.syncCurrentType = GenerateLibraryResumeData::typeFromSyncIndex(data->type),
				.syncMostRecentSave = data->syncMostRecentSave,
				.syncLastItemOffset = data->syncLastItemOffset,
				.syncLastItem = data->syncLastItem
			};
		});
		
		using YieldResult = typename LibraryItemGenerator::YieldResult;
		return LibraryItemGenerator([=]() {
			return getIdentity().then([=](Optional<SpotifyUser> user) {
				// verify spotify is logged in
				if(!user) {
					return Promise<YieldResult>::reject(SpotifyError(SpotifyError::Code::NOT_LOGGED_IN, "Spotify is not logged in"));
				}
				// if current user has changed, completely reset the sync data
				if(sharedData->userId.empty() || sharedData->userId != user->id) {
					*sharedData = SharedData();
					sharedData->userId = user->id;
				}
				
				size_t sectionCount = 3;
				auto generateMediaItems = [=](Optional<time_t>* mostRecentSave, Function<Promise<SpotifyPage<LibraryItem>>()> fetcher) {
					return fetcher().map([=](SpotifyPage<LibraryItem> page) -> YieldResult {
						// check if sync is caught up
						bool foundEndOfSync = false;
						if(mostRecentSave != nullptr && *mostRecentSave) {
							for(auto& item : page.items) {
								time_t addedAt = item.addedAt.empty() ? 0 : timeFromString(item.addedAt);
								if(addedAt < mostRecentSave->value()) {
									foundEndOfSync = true;
									break;
								}
							}
						}
						
						if(sharedData->resuming) {
							sharedData->resuming = false;
							if(!sharedData->attemptRestoreSync(page.items.first())) {
								bool done = false;
								if(mostRecentSave != nullptr) {
									if(foundEndOfSync) {
										*mostRecentSave = sharedData->syncMostRecentSave;
										sharedData->syncMostRecentSave = std::nullopt;
										sharedData->syncLastItem = std::nullopt;
										sharedData->syncLastItemOffset = std::nullopt;
										sharedData->offset = 0;
										sharedData->type += 1;
										if(sharedData->type >= sectionCount) {
											sharedData->type = 0;
											done = true;
										}
									}
								}
								return YieldResult{
									.value=GenerateLibraryResults{
										.resumeData = createResumeData(sharedData).toJson(),
										.items = page.items,
										.progress = done ? 1.0 : ((double)sharedData->type / (double)sectionCount)
									},
									.done=done
								};
							}
						}
						
						if(mostRecentSave != nullptr) {
							if(sharedData->offset == 0 && page.items.size() > 0) {
								auto latestAddedAt = page.items.front().addedAt;
								sharedData->syncMostRecentSave = latestAddedAt.empty() ? 0 : timeFromString(latestAddedAt);
							}
						} else {
							sharedData->syncMostRecentSave = std::nullopt;
						}
						
						sharedData->offset += page.items.size();
						if(page.items.size() > 0) {
							auto& lastItem = page.items.back();
							sharedData->syncLastItem = {
								.uri = lastItem.mediaItem->uri(),
								.addedAt = lastItem.addedAt
							};
							sharedData->syncLastItemOffset = sharedData->offset - 1;
						}
						
						bool done = false;
						bool sectionDone = (foundEndOfSync || sharedData->offset >= page.total || page.next.empty());
						double progress = (((double)sharedData->type + ((double)sharedData->offset / (double)page.total)) / (double)sectionCount);
						if(sectionDone) {
							// if we're done, move to the next section
							if(mostRecentSave != nullptr) {
								*mostRecentSave = sharedData->syncMostRecentSave;
							}
							sharedData->syncMostRecentSave = std::nullopt;
							sharedData->syncLastItem = std::nullopt;
							sharedData->syncLastItemOffset = std::nullopt;
							sharedData->offset = 0;
							sharedData->type += 1;
							if(sharedData->type >= sectionCount) {
								sharedData->type = 0;
								progress = 1.0;
								done = true;
							} else {
								progress = (double)sharedData->type / (double)sectionCount;
							}
						}
						return YieldResult{
							.value=GenerateLibraryResults{
								.resumeData = createResumeData(sharedData).toJson(),
								.items = page.items,
								.progress = progress
							},
							.done=done
						};
					});
				};
				
				switch(sharedData->type) {
					// saved playlists
					case 0: {
						return generateMediaItems(nullptr, [=](){
							return spotify->getMyPlaylists({
								.limit=50,
								.offset=sharedData->offset
							}).map([=](SpotifyPage<SpotifyPlaylist> page) -> SpotifyPage<LibraryItem> {
								return page.map([=](auto& item) -> LibraryItem {
									return LibraryItem{
										.libraryProvider = this,
										.mediaItem = this->playlist(createPlaylistData(item, false)),
										.addedAt = String()
									};
								});
							});
						});
					}
					
					// saved albums
					case 1: {
						return generateMediaItems(&sharedData->mostRecentAlbumSave, [=](){
							return spotify->getMyAlbums({
								.market="from_token",
								.limit=50,
								.offset=sharedData->offset
							}).map([=](SpotifyPage<SpotifySavedAlbum> page) -> SpotifyPage<LibraryItem> {
								return page.map([=](auto& item) -> LibraryItem {
									return LibraryItem{
										.libraryProvider = this,
										.mediaItem = this->album(createAlbumData(item.album, false)),
										.addedAt = item.addedAt
									};
								});
							});
						});
					}
					
					// saved tracks
					case 2: {
						return generateMediaItems(&sharedData->mostRecentTrackSave, [=](){
							return spotify->getMyTracks({
								.market="from_token",
								.limit=50,
								.offset=sharedData->offset
							}).map([=](SpotifyPage<SpotifySavedTrack> page) -> SpotifyPage<LibraryItem> {
								return page.map([=](auto& item) -> LibraryItem {
									return LibraryItem{
										.libraryProvider = this,
										.mediaItem = this->track(createTrackData(item.track, false)),
										.addedAt = item.addedAt
									};
								});
							});
						});
					}
				}
				return Promise<YieldResult>::reject(std::runtime_error("invalid spotify sync section"));
			});
		});
	}



	#pragma mark User Library - Following / Unfollowing

	Promise<void> SpotifyMediaProvider::followArtist(String artistURI) {
		auto uriParts = parseURI(artistURI);
		return spotify->followArtists({ uriParts.id }).toVoid();
	}

	Promise<void> SpotifyMediaProvider::unfollowArtist(String artistURI) {
		auto uriParts = parseURI(artistURI);
		return spotify->unfollowArtists({ uriParts.id }).toVoid();
	}

	Promise<void> SpotifyMediaProvider::followUser(String userURI) {
		auto uriParts = parseURI(userURI);
		return spotify->followUsers({ uriParts.id }).toVoid();
	}

	Promise<void> SpotifyMediaProvider::unfollowUser(String userURI) {
		auto uriParts = parseURI(userURI);
		return spotify->unfollowUsers({ uriParts.id }).toVoid();
	}

	Promise<void> SpotifyMediaProvider::saveTrack(String trackURI) {
		auto uriParts = parseURI(trackURI);
		return spotify->saveTracks({ uriParts.id });
	}

	Promise<void> SpotifyMediaProvider::unsaveTrack(String trackURI) {
		auto uriParts = parseURI(trackURI);
		return spotify->unsaveTracks({ uriParts.id });
	}

	Promise<void> SpotifyMediaProvider::saveAlbum(String albumURI) {
		auto uriParts = parseURI(albumURI);
		return spotify->saveAlbums({ uriParts.id });
	}

	Promise<void> SpotifyMediaProvider::unsaveAlbum(String albumURI) {
		auto uriParts = parseURI(albumURI);
		return spotify->unsaveAlbums({ uriParts.id });
	}



	#pragma mark Playlists

	bool SpotifyMediaProvider::canCreatePlaylists() const {
		return true;
	}

	ArrayList<Playlist::Privacy> SpotifyMediaProvider::supportedPlaylistPrivacies() const {
		return { Playlist::Privacy::PRIVATE, Playlist::Privacy::PUBLIC };
	}

	Promise<$<Playlist>> SpotifyMediaProvider::createPlaylist(String name, CreatePlaylistOptions options) {
		Optional<bool> isPublic;
		switch(options.privacy) {
			case Playlist::Privacy::UNLISTED:
			case Playlist::Privacy::PRIVATE:
				isPublic = false;
				break;
			case Playlist::Privacy::PUBLIC:
				isPublic = true;
				break;
			case Playlist::Privacy::UNKNOWN:
				break;
		}
		return spotify->createPlaylist(name, {
			.isPublic = isPublic
		}).map([=](SpotifyPlaylist playlist) -> $<Playlist> {
			return this->playlist(createPlaylistData(playlist, false));
		});
	}

	Promise<bool> SpotifyMediaProvider::isPlaylistEditable($<Playlist> playlist) {
		if(!isLoggedIn()) {
			return Promise<bool>::resolve(false);
		}
		return getIdentity().map([=](Optional<SpotifyUser> user) -> bool {
			if(!user) {
				return false;
			}
			auto owner = playlist->owner();
			return (owner != nullptr && owner->uri() == user->uri);
		});
	}

	Promise<void> SpotifyMediaProvider::deletePlaylist(String playlistURI) {
		auto uriParts = parseURI(playlistURI);
		return spotify->unfollowPlaylist(uriParts.id);
	}



	#pragma mark Player

	SpotifyPlaybackProvider* SpotifyMediaProvider::player() {
		return _player;
	}

	const SpotifyPlaybackProvider* SpotifyMediaProvider::player() const {
		return _player;
	}
}

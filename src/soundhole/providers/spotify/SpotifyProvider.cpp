//
//  SpotifyProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SpotifyProvider.hpp"
#include "mutators/SpotifyAlbumMutatorDelegate.hpp"
#include "mutators/SpotifyPlaylistMutatorDelegate.hpp"
#include <soundhole/utils/SoundHoleError.hpp>
#include <soundhole/utils/Utils.hpp>

namespace sh {
	SpotifyProvider::SpotifyProvider(Options options)
	: spotify(new Spotify(options)),
	_player(new SpotifyPlaybackProvider(this)),
	_currentUserNeedsRefresh(true) {
		//
	}

	SpotifyProvider::~SpotifyProvider() {
		delete _player;
		delete spotify;
	}



	String SpotifyProvider::name() const {
		return "spotify";
	}
	
	String SpotifyProvider::displayName() const {
		return "Spotify";
	}



	Spotify* SpotifyProvider::api() {
		return spotify;
	}

	const Spotify* SpotifyProvider::api() const {
		return spotify;
	}



	time_t SpotifyProvider::timeFromString(String time) {
		if(auto date = DateTime::fromGmtString(time, "%Y-%m-%dT%H:%M:%SZ")) {
			return date->toTimeType();
		}
		throw std::invalid_argument("Invalid date format");
	}



	#pragma mark URI parsing

	SpotifyProvider::URI SpotifyProvider::parseURI(String uri) {
		auto parts = uri.split(':');
		if(parts.size() < 2) {
			throw SoundHoleError(SoundHoleError::Code::PARSE_FAILED, "invalid spotify uri "+uri);
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

	Promise<bool> SpotifyProvider::login() {
		return spotify->login().map<bool>([=](bool loggedIn) {
			getCurrentSpotifyUser();
			return loggedIn;
		});
	}

	void SpotifyProvider::logout() {
		spotify->logout();
		setCurrentSpotifyUser(std::nullopt);
	}

	bool SpotifyProvider::isLoggedIn() const {
		return spotify->isLoggedIn();
	}



	#pragma mark Current User

	Promise<ArrayList<String>> SpotifyProvider::getCurrentUserIds() {
		return getCurrentSpotifyUser().map<ArrayList<String>>([=](Optional<SpotifyUser> user) -> ArrayList<String> {
			if(!user) {
				return {};
			}
			return { user->id };
		});
	}

	String SpotifyProvider::getCachedCurrentSpotifyUserPath() const {
		String sessionPersistKey = spotify->getAuth()->getOptions().sessionPersistKey;
		if(sessionPersistKey.empty()) {
			return String();
		}
		return utils::getCacheDirectoryPath()+"/"+name()+"_user_"+sessionPersistKey+".json";
	}

	Promise<Optional<SpotifyUser>> SpotifyProvider::getCurrentSpotifyUser() {
		if(!isLoggedIn()) {
			setCurrentSpotifyUser(std::nullopt);
			return Promise<Optional<SpotifyUser>>::resolve(std::nullopt);
		}
		if(_currentUserPromise) {
			return _currentUserPromise.value();
		}
		if(_currentUser && !_currentUserNeedsRefresh) {
			return Promise<Optional<SpotifyUser>>::resolve(_currentUser);
		}
		auto promise = spotify->getMe().map<Optional<SpotifyUser>>([=](SpotifyUser meUser) -> Optional<SpotifyUser> {
			_currentUserPromise = std::nullopt;
			if(!isLoggedIn()) {
				setCurrentSpotifyUser(std::nullopt);
				return std::nullopt;
			}
			setCurrentSpotifyUser(meUser);
			return meUser;
		}).except([=](std::exception_ptr error) -> Optional<SpotifyUser> {
			_currentUserPromise = std::nullopt;
			// if current user is loaded, return it
			if(_currentUser) {
				return _currentUser.value();
			}
			// try to load user from filesystem
			auto userFilePath = getCachedCurrentSpotifyUserPath();
			if(!userFilePath.empty()) {
				try {
					String userString = fs::readFile(userFilePath);
					std::string jsonError;
					Json userJson = Json::parse(userString, jsonError);
					if(userJson.is_null()) {
						throw std::runtime_error("failed to parse user json: "+jsonError);
					}
					auto user = SpotifyUser::fromJson(userJson);
					_currentUser = user;
					_currentUserNeedsRefresh = true;
					return user;
				} catch(...) {
					// ignore error
				}
			}
			// rethrow error
			std::rethrow_exception(error);
		});
		_currentUserPromise = promise;
		if(_currentUserPromise && _currentUserPromise->isComplete()) {
			_currentUserPromise = std::nullopt;
		}
		return promise;
	}

	void SpotifyProvider::setCurrentSpotifyUser(Optional<SpotifyUser> user) {
		auto userFilePath = getCachedCurrentSpotifyUserPath();
		if(user) {
			_currentUser = user;
			_currentUserNeedsRefresh = false;
			if(!userFilePath.empty()) {
				fs::writeFile(userFilePath, user->toJson().dump());
			}
		} else {
			_currentUser = std::nullopt;
			_currentUserPromise = std::nullopt;
			_currentUserNeedsRefresh = true;
			if(!userFilePath.empty() && fs::exists(userFilePath)) {
				fs::remove(userFilePath);
			}
		}
	}



	#pragma mark Search

	Promise<SpotifyProvider::SearchResults> SpotifyProvider::search(String query, SearchOptions options) {
		Spotify::SearchOptions searchOptions = {
			.types=options.types,
			.market="from_token",
			.limit=options.limit,
			.offset=options.offset
		};
		return spotify->search(query,searchOptions).map<SpotifyProvider::SearchResults>([=](auto searchResults) {
			return SearchResults{
				.tracks = searchResults.tracks ?
					maybe(searchResults.tracks->template map<$<Track>>([&](auto& track) {
						return Track::new$(this, createTrackData(track, false));
					}))
					: std::nullopt,
				.albums = searchResults.albums ?
					maybe(searchResults.albums->template map<$<Album>>([&](auto& album) {
						return Album::new$(this, createAlbumData(album, true));
					}))
					: std::nullopt,
				.artists = searchResults.artists ?
					maybe(searchResults.artists->template map<$<Artist>>([&](auto& artist) {
						return Artist::new$(this, createArtistData(artist, false));
					}))
					: std::nullopt,
				.playlists = searchResults.playlists ?
					maybe(searchResults.playlists->template map<$<Playlist>>([&](auto& playlist) {
						return Playlist::new$(this, createPlaylistData(playlist, true));
					}))
					: std::nullopt,
			};
		});
	}



	#pragma mark Data transforming

	Track::Data SpotifyProvider::createTrackData(SpotifyTrack track, bool partial) {
		return Track::Data{{
			.partial=partial,
			.type=track.type,
			.name=track.name,
			.uri=track.uri,
			.images=(track.album ?
				maybe(track.album->images.map<MediaItem::Image>([&](SpotifyImage& image) {
					return createImage(std::move(image));
				}))
				: std::nullopt)
			},
			.albumName=(track.album ? track.album->name : ""),
			.albumURI=(track.album ? track.album->uri : ""),
			.artists=track.artists.map<$<Artist>>([&](SpotifyArtist& artist) {
				return Artist::new$(this, createArtistData(std::move(artist), true));
			}),
			.tags=ArrayList<String>(),
			.discNumber=track.discNumber,
			.trackNumber=track.trackNumber,
			.duration=(((double)track.durationMs)/1000.0),
			.audioSources=std::nullopt,
			.playable=track.isPlayable.value_or(!(track.availableMarkets && track.availableMarkets->size() == 0))
		};
	}

	Artist::Data SpotifyProvider::createArtistData(SpotifyArtist artist, bool partial) {
		return Artist::Data{{
			.partial=partial,
			.type=artist.type,
			.name=artist.name,
			.uri=artist.uri,
			.images=(artist.images ?
				maybe(artist.images->map<MediaItem::Image>([&](SpotifyImage& image) {
					return createImage(std::move(image));
				}))
				: std::nullopt)
			},
			.description=""
		};
	}

	Album::Data SpotifyProvider::createAlbumData(SpotifyAlbum album, bool partial) {
		auto artists = album.artists.map<$<Artist>>([&](SpotifyArtist& artist) {
			return Artist::new$(this, createArtistData(artist, true));
		});
		auto albumImages = album.images.map<MediaItem::Image>([&](SpotifyImage& image) {
			return createImage(std::move(image));
		});
		return Album::Data{{{
			.partial=partial,
			.type=album.type,
			.name=album.name,
			.uri=album.uri,
			.images=albumImages
			},
			.versionId=album.releaseDate,
			.tracks=(album.tracks ?
				maybe(Album::Data::Tracks{
					.total=album.tracks->total,
					.offset=album.tracks->offset,
					.items=album.tracks->items.map<AlbumItem::Data>([&](SpotifyTrack& track) {
						auto trackData = createTrackData(track, true);
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
						return AlbumItem::Data{
							.track=Track::new$(this, trackData)
						};
					})
				})
				: std::nullopt)
			},
			.artists=artists
		};
	}

	Playlist::Data SpotifyProvider::createPlaylistData(SpotifyPlaylist playlist, bool partial) {
		return Playlist::Data{{{
			.partial=partial,
			.type=playlist.type,
			.name=playlist.name,
			.uri=playlist.uri,
			.images=playlist.images.map<MediaItem::Image>([&](SpotifyImage& image) {
				return createImage(std::move(image));
			})
			},
			.versionId=playlist.snapshotId,
			.tracks=Playlist::Data::Tracks{
				.total=playlist.tracks.total,
				.offset=playlist.tracks.offset,
				.items=playlist.tracks.items.map<PlaylistItem::Data>([&](SpotifyPlaylist::Item& item) {
					return PlaylistItem::Data{{
						.track=Track::new$(this, createTrackData(item.track, true))
						},
						.addedAt=item.addedAt,
						.addedBy=UserAccount::new$(this, createUserAccountData(item.addedBy, true))
					};
				})
			}
			},
			.owner=UserAccount::new$(this, createUserAccountData(playlist.owner, true)),
			.privacy=(playlist.isPublic ?
				(playlist.isPublic.value() ? Playlist::Privacy::PUBLIC : Playlist::Privacy::PRIVATE)
				: Playlist::Privacy::UNKNOWN)
		};
	}

	PlaylistItem::Data SpotifyProvider::createPlaylistItemData(SpotifyPlaylist::Item playlistItem) {
		return PlaylistItem::Data{{
			.track=Track::new$(this, createTrackData(playlistItem.track, true))
			},
			.addedAt=playlistItem.addedAt,
			.addedBy=UserAccount::new$(this, createUserAccountData(playlistItem.addedBy, true))
		};
	}

	UserAccount::Data SpotifyProvider::createUserAccountData(SpotifyUser user, bool partial) {
		return UserAccount::Data{{
			.partial=partial,
			.type=user.type,
			.name=user.displayName.value_or(user.id),
			.uri=user.uri,
			.images=(user.images ? maybe(user.images->map<MediaItem::Image>([&](SpotifyImage& image) {
				return createImage(std::move(image));
			})) : std::nullopt)
			},
			.id=user.id,
			.displayName=user.displayName
		};
	}

	MediaItem::Image SpotifyProvider::createImage(SpotifyImage image) {
		auto dimensions = MediaItem::Image::Dimensions{ image.width, image.height };
		return MediaItem::Image{
			.url=image.url,
			.size=dimensions.toSize(),
			.dimensions=dimensions
		};
	}



	#pragma mark Media Item Fetching

	Promise<Track::Data> SpotifyProvider::getTrackData(String uri) {
		auto uriParts = parseURI(uri);
		return spotify->getTrack(uriParts.id,{.market="from_token"}).map<Track::Data>([=](SpotifyTrack track) {
			return createTrackData(track, false);
		});
	}

	Promise<Artist::Data> SpotifyProvider::getArtistData(String uri) {
		auto uriParts = parseURI(uri);
		return spotify->getArtist(uriParts.id).map<Artist::Data>([=](SpotifyArtist artist) {
			return createArtistData(artist, false);
		});
	}

	Promise<Album::Data> SpotifyProvider::getAlbumData(String uri) {
		auto uriParts = parseURI(uri);
		return spotify->getAlbum(uriParts.id,{.market="from_token"}).map<Album::Data>([=](SpotifyAlbum album) {
			return createAlbumData(album, false);
		});
	}

	Promise<Playlist::Data> SpotifyProvider::getPlaylistData(String uri) {
		auto uriParts = parseURI(uri);
		return spotify->getPlaylist(uriParts.id,{.market="from_token"}).map<Playlist::Data>([=](SpotifyPlaylist playlist) {
			return createPlaylistData(playlist, false);
		});
	}

	Promise<UserAccount::Data> SpotifyProvider::getUserData(String uri) {
		auto uriParts = parseURI(uri);
		return spotify->getUser(uriParts.id).map<UserAccount::Data>([=](SpotifyUser user) {
			return createUserAccountData(user, false);
		});
	}




	Promise<ArrayList<$<Track>>> SpotifyProvider::getArtistTopTracks(String artistURI) {
		auto uriParts = parseURI(artistURI);
		return spotify->getArtistTopTracks(uriParts.id,"from_token").map<ArrayList<$<Track>>>(nullptr, [=](ArrayList<SpotifyTrack> tracks) {
			return tracks.map<$<Track>>([=](auto track) {
				return Track::new$(this, createTrackData(track, false));
			});
		});
	}

	SpotifyProvider::ArtistAlbumsGenerator SpotifyProvider::getArtistAlbums(String artistURI) {
		auto uriParts = parseURI(artistURI);
		auto offset = fgl::new$<size_t>(0);
		using YieldResult = typename ArtistAlbumsGenerator::YieldResult;
		return ArtistAlbumsGenerator([=]() {
			return spotify->getArtistAlbums(uriParts.id, {
				.country="from_token",
				.limit=20,
				.offset=*offset
			}).map<YieldResult>([=](auto page) {
				auto loadBatch = LoadBatch<$<Album>>{
					.items=page.items.template map<$<Album>>([=](auto album) {
						return Album::new$(this, createAlbumData(album, true));
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

	SpotifyProvider::UserPlaylistsGenerator SpotifyProvider::getUserPlaylists(String userURI) {
		auto uriParts = parseURI(userURI);
		auto offset = fgl::new$<size_t>(0);
		using YieldResult = typename UserPlaylistsGenerator::YieldResult;
		return UserPlaylistsGenerator([=]() {
			return spotify->getUserPlaylists(uriParts.id, {
				.limit=20,
				.offset=*offset
			}).map<YieldResult>([=](auto page) {
				auto loadBatch = LoadBatch<$<Playlist>>{
					.items=page.items.template map<$<Playlist>>([=](auto playlist) {
						return Playlist::new$(this, createPlaylistData(playlist, true));
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




	Album::MutatorDelegate* SpotifyProvider::createAlbumMutatorDelegate($<Album> album) {
		return new SpotifyAlbumMutatorDelegate(album);
	}

	Playlist::MutatorDelegate* SpotifyProvider::createPlaylistMutatorDelegate($<Playlist> playlist) {
		return new SpotifyPlaylistMutatorDelegate(playlist);
	}



	#pragma mark User Library

	SpotifyProvider::GenerateLibraryResumeData SpotifyProvider::GenerateLibraryResumeData::fromJson(const Json& json) {
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

	Json SpotifyProvider::GenerateLibraryResumeData::toJson() const {
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

	String SpotifyProvider::GenerateLibraryResumeData::typeFromSyncIndex(size_t index) {
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

	Optional<size_t> SpotifyProvider::GenerateLibraryResumeData::syncIndexFromType(String type) {
		if(type == "playlists") {
			return 0;
		} else if(type == "albums") {
			return 1;
		} else if(type == "tracks") {
			return 2;
		}
		return std::nullopt;
	}

	Optional<SpotifyProvider::GenerateLibraryResumeData::Item> SpotifyProvider::GenerateLibraryResumeData::Item::maybeFromJson(const Json& json) {
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

	Json SpotifyProvider::GenerateLibraryResumeData::Item::toJson() const {
		return Json::object{
			{ "uri", Json(uri) },
			{ "addedAt", Json(addedAt) }
		};
	}

	bool SpotifyProvider::hasLibrary() const {
		return true;
	}

	SpotifyProvider::LibraryItemGenerator SpotifyProvider::generateLibrary(GenerateLibraryOptions options) {
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
			return getCurrentSpotifyUser().then([=](Optional<SpotifyUser> user) {
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
					return fetcher().map<YieldResult>([=](SpotifyPage<LibraryItem> page) {
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
							}).map<SpotifyPage<LibraryItem>>([=](SpotifyPage<SpotifyPlaylist> page) {
								return page.map<LibraryItem>([=](auto& item) {
									return LibraryItem{
										.libraryProvider = this,
										.mediaItem = Playlist::new$(this, createPlaylistData(item, false)),
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
							}).map<SpotifyPage<LibraryItem>>([=](SpotifyPage<SpotifySavedAlbum> page) {
								return page.map<LibraryItem>([=](auto& item) {
									return LibraryItem{
										.libraryProvider = this,
										.mediaItem = Album::new$(this, createAlbumData(item.album, false)),
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
							}).map<SpotifyPage<LibraryItem>>([=](SpotifyPage<SpotifySavedTrack> page) {
								return page.map<LibraryItem>([=](auto& item) {
									return LibraryItem{
										.libraryProvider = this,
										.mediaItem = Track::new$(this, createTrackData(item.track, false)),
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



	#pragma mark Playlists

	bool SpotifyProvider::canCreatePlaylists() const {
		return true;
	}

	ArrayList<Playlist::Privacy> SpotifyProvider::supportedPlaylistPrivacies() const {
		return { Playlist::Privacy::PRIVATE, Playlist::Privacy::PUBLIC };
	}

	Promise<$<Playlist>> SpotifyProvider::createPlaylist(String name, CreatePlaylistOptions options) {
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
		}).map<$<Playlist>>([=](SpotifyPlaylist playlist) {
			return Playlist::new$(this, createPlaylistData(playlist, false));
		});
	}



	Promise<bool> SpotifyProvider::isPlaylistEditable($<Playlist> playlist) {
		if(!isLoggedIn()) {
			return Promise<bool>::resolve(false);
		}
		return getCurrentSpotifyUser().map<bool>([=](Optional<SpotifyUser> user) -> bool {
			if(!user) {
				return false;
			}
			auto owner = playlist->owner();
			return (owner != nullptr && owner->id() == user->id);
		});
	}



	#pragma mark Player

	SpotifyPlaybackProvider* SpotifyProvider::player() {
		return _player;
	}

	const SpotifyPlaybackProvider* SpotifyProvider::player() const {
		return _player;
	}
}

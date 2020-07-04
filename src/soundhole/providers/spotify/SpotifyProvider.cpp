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

namespace sh {
	SpotifyProvider::SpotifyProvider(Options options)
	: spotify(new Spotify(options)),
	_player(new SpotifyPlaybackProvider(this)) {
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




	Promise<bool> SpotifyProvider::login() {
		return spotify->login();
	}

	void SpotifyProvider::logout() {
		spotify->logout();
	}

	bool SpotifyProvider::isLoggedIn() const {
		return spotify->isLoggedIn();
	}




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



	String SpotifyProvider::idFromURI(String uri) {
		auto parts = uri.split(':');
		if(parts.size() != 3) {
			return uri;
		}
		return parts.back();
	}

	time_t SpotifyProvider::timeFromString(String time) {
		tm timeData;
		strptime(time.c_str(), "%Y-%m-%dT%H:%M:%SZ", &timeData);
		return mktime(&timeData);
	}



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
			.owner=UserAccount::new$(this, createUserAccountData(playlist.owner, true))
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



	Promise<Track::Data> SpotifyProvider::getTrackData(String uri) {
		return spotify->getTrack(idFromURI(uri),{.market="from_token"}).map<Track::Data>([=](SpotifyTrack track) {
			return createTrackData(track, false);
		});
	}

	Promise<Artist::Data> SpotifyProvider::getArtistData(String uri) {
		return spotify->getArtist(idFromURI(uri)).map<Artist::Data>([=](SpotifyArtist artist) {
			return createArtistData(artist, false);
		});
	}

	Promise<Album::Data> SpotifyProvider::getAlbumData(String uri) {
		return spotify->getAlbum(idFromURI(uri),{.market="from_token"}).map<Album::Data>([=](SpotifyAlbum album) {
			return createAlbumData(album, false);
		});
	}

	Promise<Playlist::Data> SpotifyProvider::getPlaylistData(String uri) {
		return spotify->getPlaylist(idFromURI(uri),{.market="from_token"}).map<Playlist::Data>([=](SpotifyPlaylist playlist) {
			return createPlaylistData(playlist, false);
		});
	}

	Promise<UserAccount::Data> SpotifyProvider::getUserData(String uri) {
		return spotify->getUser(idFromURI(uri)).map<UserAccount::Data>([=](SpotifyUser user) {
			return createUserAccountData(user, false);
		});
	}




	Promise<ArrayList<$<Track>>> SpotifyProvider::getArtistTopTracks(String artistURI) {
		return spotify->getArtistTopTracks(idFromURI(artistURI),"from_token").map<ArrayList<$<Track>>>(nullptr, [=](ArrayList<SpotifyTrack> tracks) {
			return tracks.map<$<Track>>([=](auto track) {
				return Track::new$(this, createTrackData(track, false));
			});
		});
	}

	SpotifyProvider::ArtistAlbumsGenerator SpotifyProvider::getArtistAlbums(String artistURI) {
		String artistId = idFromURI(artistURI);
		auto offset = fgl::new$<size_t>(0);
		using YieldResult = typename ArtistAlbumsGenerator::YieldResult;
		return ArtistAlbumsGenerator([=]() {
			return spotify->getArtistAlbums(artistId, {
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
		String userId = idFromURI(userURI);
		auto offset = fgl::new$<size_t>(0);
		using YieldResult = typename UserPlaylistsGenerator::YieldResult;
		return UserPlaylistsGenerator([=]() {
			return spotify->getUserPlaylists(userId, {
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




	SpotifyProvider::GenerateLibraryResumeData SpotifyProvider::GenerateLibraryResumeData::fromJson(const Json& json) {
		auto mostRecentTrackSave = json["mostRecentTrackSave"];
		auto mostRecentAlbumSave = json["mostRecentAlbumSave"];
		auto syncMostRecentSave = json["syncMostRecentSave"];
		auto syncLastItemOffset = json["syncLastItemOffset"];
		auto resumeData = GenerateLibraryResumeData{
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
			Optional<time_t> mostRecentTrackSave;
			Optional<time_t> mostRecentAlbumSave;
			Optional<GenerateLibraryResumeData::Item> syncLastItem;
			Optional<size_t> syncLastItemOffset;
			Optional<time_t> syncMostRecentSave;
			size_t type;
			size_t offset;
			bool resuming;
			
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
			.mostRecentTrackSave = resumeData.mostRecentTrackSave,
			.mostRecentAlbumSave = resumeData.mostRecentAlbumSave,
			.syncLastItem = resumeData.syncLastItem,
			.syncLastItemOffset = resumeData.syncLastItemOffset,
			.syncMostRecentSave = resumeData.syncMostRecentSave,
			.type = GenerateLibraryResumeData::syncIndexFromType(resumeData.syncCurrentType).value_or(0),
			.offset = resumeData.syncLastItemOffset.value_or(0),
			.resuming = (resumeData.syncLastItem.has_value() && resumeData.syncLastItemOffset.value_or(0) > 0)
		});
		auto createResumeData = std::function<GenerateLibraryResumeData($<SharedData>)>([]($<SharedData> data) {
			return GenerateLibraryResumeData{
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
			switch(sharedData->type) {
				// saved playlists
				case 0: {
					return spotify->getMyPlaylists({
						.limit=50,
						.offset=sharedData->offset
					}).map<YieldResult>([=](auto page) {
						auto items = page.items.template map<LibraryItem>([=](auto item) {
							return LibraryItem{
								.libraryProvider = this,
								.mediaItem = Playlist::new$(this, createPlaylistData(item, false)),
								.addedAt = String()
							};
						});
						
						if(sharedData->resuming) {
							sharedData->resuming = false;
							if(!sharedData->attemptRestoreSync(items.first())) {
								return YieldResult{
									.value=GenerateLibraryResults{
										.resumeData = createResumeData(sharedData).toJson(),
										.items = items,
										.progress = 0
									},
									.done=false
								};
							}
						}
						
						sharedData->offset += items.size();
						if(items.size() > 0) {
							auto& lastItem = items.back();
							sharedData->syncLastItem = {
								.uri = lastItem.mediaItem->uri(),
								.addedAt = lastItem.addedAt
							};
							sharedData->syncLastItemOffset = sharedData->offset - 1;
						}
						sharedData->syncMostRecentSave = std::nullopt;
						bool done = (sharedData->offset >= page.total || page.next.empty());
						if(done) {
							sharedData->syncLastItem = std::nullopt;
							sharedData->syncLastItemOffset = std::nullopt;
							sharedData->offset = 0;
							sharedData->type += 1;
						}
						return YieldResult{
							.value=GenerateLibraryResults{
								.resumeData = createResumeData(sharedData).toJson(),
								.items = items,
								.progress = (done ? (1.0 / 3.0) : ((0.0 + ((double)sharedData->offset / (double)page.total)) / 3.0))
							},
							.done=false
						};
					});
				}
				
				// saved albums
				case 1: {
					return spotify->getMyAlbums({
						.market="from_token",
						.limit=50,
						.offset=sharedData->offset
					}).map<YieldResult>([=](auto page) {
						auto items = page.items.template map<LibraryItem>([=](auto item) {
							return LibraryItem{
								.libraryProvider = this,
								.mediaItem = Album::new$(this, createAlbumData(item.album, false)),
								.addedAt = item.addedAt
							};
						});
						
						bool foundEndOfSync = false;
						if(sharedData->mostRecentAlbumSave) {
							for(auto& item : items) {
								time_t addedAt = item.addedAt.empty() ? 0 : timeFromString(item.addedAt);
								if(addedAt < sharedData->mostRecentAlbumSave.value()) {
									foundEndOfSync = true;
									break;
								}
							}
						}
						
						if(sharedData->resuming) {
							sharedData->resuming = false;
							if(!sharedData->attemptRestoreSync(items.first())) {
								if(foundEndOfSync) {
									sharedData->mostRecentAlbumSave = sharedData->syncMostRecentSave;
									sharedData->syncMostRecentSave = std::nullopt;
									sharedData->syncLastItem = std::nullopt;
									sharedData->syncLastItemOffset = std::nullopt;
									sharedData->offset = 0;
									sharedData->type += 1;
								}
								return YieldResult{
									.value=GenerateLibraryResults{
										.resumeData = createResumeData(sharedData).toJson(),
										.items = items,
										.progress = foundEndOfSync ? (2.0 / 3.0) : (1.0 / 3.0)
									},
									.done=false
								};
							}
						}
						
						if(sharedData->offset == 0 && items.size() > 0) {
							auto latestAddedAt = items.front().addedAt;
							sharedData->syncMostRecentSave = latestAddedAt.empty() ? 0 : timeFromString(latestAddedAt);
						}
						sharedData->offset += items.size();
						if(items.size() > 0) {
							auto& lastItem = items.back();
							sharedData->syncLastItem = {
								.uri = lastItem.mediaItem->uri(),
								.addedAt = lastItem.addedAt
							};
							sharedData->syncLastItemOffset = sharedData->offset - 1;
						}
						bool done = (foundEndOfSync || sharedData->offset >= page.total || page.next.empty());
						if(done) {
							sharedData->mostRecentAlbumSave = sharedData->syncMostRecentSave;
							sharedData->syncMostRecentSave = std::nullopt;
							sharedData->syncLastItem = std::nullopt;
							sharedData->syncLastItemOffset = std::nullopt;
							sharedData->offset = 0;
							sharedData->type += 1;
						}
						return YieldResult{
							.value=GenerateLibraryResults{
								.resumeData = createResumeData(sharedData).toJson(),
								.items = items,
								.progress = (done ? (2.0 / 3.0) : ((1.0 + ((double)sharedData->offset / (double)page.total)) / 3.0))
							},
							.done=false
						};
					});
				}
				
				// saved tracks
				case 2: {
					return spotify->getMyTracks({
						.market="from_token",
						.limit=50,
						.offset=sharedData->offset
					}).map<YieldResult>([=](auto page) {
						auto items = page.items.template map<LibraryItem>([=](auto item) {
							return LibraryItem{
								.libraryProvider = this,
								.mediaItem = Track::new$(this, createTrackData(item.track, false)),
								.addedAt = item.addedAt
							};
						});
						
						bool foundEndOfSync = false;
						if(sharedData->mostRecentTrackSave) {
							for(auto& item : items) {
								time_t addedAt = item.addedAt.empty() ? 0 : timeFromString(item.addedAt);
								if(addedAt < sharedData->mostRecentTrackSave.value()) {
									foundEndOfSync = true;
									break;
								}
							}
						}
						
						if(sharedData->resuming) {
							sharedData->resuming = false;
							if(!sharedData->attemptRestoreSync(items.first())) {
								if(foundEndOfSync) {
									sharedData->mostRecentTrackSave = sharedData->syncMostRecentSave;
									sharedData->syncMostRecentSave = std::nullopt;
									sharedData->syncLastItem = std::nullopt;
									sharedData->syncLastItemOffset = std::nullopt;
									sharedData->offset = 0;
									sharedData->type += 1;
								}
								return YieldResult{
									.value=GenerateLibraryResults{
										.resumeData = createResumeData(sharedData).toJson(),
										.items = items,
										.progress = foundEndOfSync ? (3.0 / 3.0) : (2.0 / 3.0)
									},
									.done=false
								};
							}
						}
						
						if(sharedData->offset == 0 && items.size() > 0) {
							auto latestAddedAt = items.front().addedAt;
							sharedData->syncMostRecentSave = latestAddedAt.empty() ? 0 : timeFromString(latestAddedAt);
						}
						sharedData->offset += items.size();
						if(items.size() > 0) {
							auto& lastItem = items.back();
							sharedData->syncLastItem = {
								.uri = lastItem.mediaItem->uri(),
								.addedAt = lastItem.addedAt
							};
							sharedData->syncLastItemOffset = sharedData->offset - 1;
						}
						bool done = (foundEndOfSync || sharedData->offset >= page.total || page.next.empty());
						if(done) {
							sharedData->mostRecentTrackSave = sharedData->syncMostRecentSave;
							sharedData->syncMostRecentSave = std::nullopt;
							sharedData->syncLastItem = std::nullopt;
							sharedData->syncLastItemOffset = std::nullopt;
							sharedData->offset = 0;
							sharedData->type += 1;
						}
						return YieldResult{
							.value=GenerateLibraryResults{
								.resumeData = createResumeData(sharedData).toJson(),
								.items = items,
								.progress = (done ? 1.0 : ((2.0 + ((double)sharedData->offset / (double)page.total)) / 3.0))
							},
							.done=false
						};
					});
				}
			}
			return Promise<YieldResult>::resolve(YieldResult{
				.done=true
			});
		});
	}




	SpotifyPlaybackProvider* SpotifyProvider::player() {
		return _player;
	}

	const SpotifyPlaybackProvider* SpotifyProvider::player() const {
		return _player;
	}
}

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
		return Album::Data{{{
			.partial=partial,
			.type=album.type,
			.name=album.name,
			.uri=album.uri,
			.images=album.images.map<MediaItem::Image>([&](SpotifyImage& image) {
				return createImage(std::move(image));
			})
			},
			.tracks=(album.tracks ?
				maybe(Album::Data::Tracks{
					.total=album.tracks->total,
					.offset=album.tracks->offset,
					.items=album.tracks->items.map<AlbumItem::Data>([&](SpotifyTrack& track) {
						auto trackData = createTrackData(track, true);
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
		return Promise<UserAccount::Data>::reject(std::logic_error("This method is not implemented"));
	}




	Promise<ArrayList<$<Track>>> SpotifyProvider::getArtistTopTracks(String artistURI) {
		return spotify->getArtistTopTracks(idFromURI(artistURI),"from_token").map<ArrayList<$<Track>>>(nullptr, [=](ArrayList<SpotifyTrack> tracks) {
			return tracks.map<$<Track>>([=](auto track) {
				return Track::new$(this, createTrackData(track, false));
			});
		});
	}

	ContinuousGenerator<SpotifyProvider::LoadBatch<$<Album>>,void> SpotifyProvider::getArtistAlbums(String artistURI) {
		String artistId = idFromURI(artistURI);
		struct SharedData {
			size_t offset = 0;
		};
		auto sharedData = fgl::new$<SharedData>();
		using YieldResult = typename ContinuousGenerator<LoadBatch<$<Album>>,void>::YieldResult;
		return ContinuousGenerator<LoadBatch<$<Album>>,void>([=]() {
			return spotify->getArtistAlbums(artistId, {
				.country="from_token",
				.offset=sharedData->offset
			}).map<YieldResult>([=](auto page) {
				auto loadBatch = LoadBatch<$<Album>>{
					.items=page.items.template map<$<Album>>([=](auto album) {
						return Album::new$(this, createAlbumData(album,true));
					}),
					.total=page.total
				};
				sharedData->offset += page.items.size();
				bool done = (sharedData->offset >= page.total);
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




	SpotifyPlaybackProvider* SpotifyProvider::player() {
		return _player;
	}

	const SpotifyPlaybackProvider* SpotifyProvider::player() const {
		return _player;
	}
}

//
//  SpotifyProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SpotifyProvider.hpp"
#include <soundhole/utils/Utils.hpp>

namespace sh {
	SpotifyProvider::SpotifyProvider(Options options)
	: spotify(new Spotify(options)) {
		//
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



	String SpotifyProvider::idFromURI(String uri) {
		auto parts = uri.split(':');
		if(parts.size() != 3) {
			return uri;
		}
		return parts.back();
	}

	Track::Data SpotifyProvider::createTrackData(SpotifyTrack track) {
		return Track::Data{{
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
				return Artist::new$(this, createArtistData(std::move(artist)));
			}),
			.tags=ArrayList<String>(),
			.discNumber=track.discNumber,
			.trackNumber=track.trackNumber,
			.duration=(((double)track.durationMs)/1000.0),
			.audioSources=std::nullopt
		};
	}

	Artist::Data SpotifyProvider::createArtistData(SpotifyArtist artist) {
		return Artist::Data{{
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

	Album::Data SpotifyProvider::createAlbumData(SpotifyAlbum album) {
		return Album::Data{{{
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
						return AlbumItem::Data{
							.track=Track::new$(this, createTrackData(track))
						};
					})
				})
				: std::nullopt)
			},
			.artists=album.artists.map<$<Artist>>([&](SpotifyArtist& artist) {
				return Artist::new$(this, createArtistData(artist));
			})
		};
	}

	Playlist::Data SpotifyProvider::createPlaylistData(SpotifyPlaylist playlist) {
		return Playlist::Data{{{
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
						.track=Track::new$(this, createTrackData(item.track))
						},
						.addedAt=utils::stringToTime(item.addedAt)
					};
				})
			}
			},
			.owner=UserAccount::new$(this, createUserAccountData(playlist.owner))
		};
	}

	UserAccount::Data SpotifyProvider::createUserAccountData(SpotifyUser user) {
		return UserAccount::Data{{
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
		return spotify->getTrack(idFromURI(uri)).map<Track::Data>([=](SpotifyTrack track) {
			return createTrackData(track);
		});
	}

	Promise<Artist::Data> SpotifyProvider::getArtistData(String uri) {
		return spotify->getArtist(idFromURI(uri)).map<Artist::Data>([=](SpotifyArtist artist) {
			return createArtistData(artist);
		});
	}

	Promise<Album::Data> SpotifyProvider::getAlbumData(String uri) {
		return spotify->getAlbum(idFromURI(uri)).map<Album::Data>([=](SpotifyAlbum album) {
			return createAlbumData(album);
		});
	}

	Promise<Playlist::Data> SpotifyProvider::getPlaylistData(String uri) {
		return spotify->getPlaylist(idFromURI(uri)).map<Playlist::Data>([=](SpotifyPlaylist playlist) {
			return createPlaylistData(playlist);
		});
	}



	bool SpotifyProvider::usesPublicAudioStreams() const {
		return false;
	}
}

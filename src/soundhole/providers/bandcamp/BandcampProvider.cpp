//
//  BandcampProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "BandcampProvider.hpp"

namespace sh {
	BandcampProvider::BandcampProvider()
	: bandcamp(new Bandcamp()) {
		//
	}

	BandcampProvider::~BandcampProvider() {
		delete bandcamp;
	}

	String BandcampProvider::name() const {
		return "bandcamp";
	}

	String BandcampProvider::displayName() const {
		return "Bandcamp";
	}




	Promise<bool> BandcampProvider::login() {
		// TODO implement login
		return Promise<bool>::reject(std::logic_error("Bandcamp login is not available"));
	}

	void BandcampProvider::logout() {
		// TODO implement logout
	}

	bool BandcampProvider::isLoggedIn() const {
		return false;
	}




	Track::Data BandcampProvider::createTrackData(BandcampTrack track) {
		return Track::Data{{
			.type="track",
			.name=track.name,
			.uri=track.url,
			.images=track.images.map<MediaItem::Image>([=](auto& image) {
				return createImage(image);
			})
			},
			.albumName=track.albumName,
			.albumURI=track.albumURL,
			.artists=ArrayList<$<Artist>>{
				Artist::new$(this,
					track.artist ?
					createArtistData(track.artist.value())
					: Artist::Data{{
						.type="artist",
						.name=track.artistName,
						.uri=track.artistURL,
						.images=std::nullopt
						},
						.description=std::nullopt
					}
				)
			},
			.tags=track.tags,
			.discNumber=std::nullopt,
			.trackNumber=std::nullopt,
			.duration=track.duration,
			.audioSources=(track.playable.has_value() ?
				maybe(track.playable.value() ? ArrayList<Track::AudioSource>{
					Track::AudioSource{
						.url=track.audioURL.value(),
						.encoding="mp3",
						.bitrate=128
					}
				} : ArrayList<Track::AudioSource>{})
				: std::nullopt)
		};
	}

	Artist::Data BandcampProvider::createArtistData(BandcampArtist artist) {
		return Artist::Data{{
			.type="artist",
			.name=artist.name,
			.uri=artist.url,
			.images=artist.images.map<MediaItem::Image>([=](auto& image) {
				return createImage(image);
			})
			},
			.description=artist.description
		};
	}

	Album::Data BandcampProvider::createAlbumData(BandcampAlbum album) {
		auto artist = Artist::new$(this,
			album.artist ?
			createArtistData(album.artist.value())
			: Artist::Data{{
				.type="artist",
				.name=album.artistName,
				.uri=album.artistURL,
				.images=std::nullopt
				},
				.description=std::nullopt
			}
		);
		return Album::Data{{{
			.type="album",
			.name=album.name,
			.uri=album.url,
			.images=album.images.map<MediaItem::Image>([&](auto& image) {
				return createImage(std::move(image));
			})
			},
			.tracks=(album.tracks ?
				maybe(Album::Data::Tracks{
					.total=album.tracks->size(),
					.offset=0,
					.items=album.tracks->map<AlbumItem::Data>([&](auto& track) {
						auto trackData = createTrackData(track);
						if(trackData.artists.size() == 0) {
							trackData.artists = { artist };
						} else {
							for(size_t i=0; i<trackData.artists.size(); i++) {
								auto cmpArtist = trackData.artists[i];
								if(cmpArtist->uri() == artist->uri()) {
									trackData.artists[i] = artist;
									break;
								}
							}
						}
						return AlbumItem::Data{
							.track=Track::new$(this, trackData)
						};
					})
				})
				: std::nullopt)
			},
			.artists=ArrayList<$<Artist>>{
				artist
			},
		};
	}

	MediaItem::Image BandcampProvider::createImage(BandcampImage image) {
		return MediaItem::Image{
			.url=image.url,
			.size=([&]() {
				switch(image.size) {
					case BandcampImage::Size::SMALL:
						return MediaItem::Image::Size::SMALL;
					case BandcampImage::Size::MEDIUM:
						return MediaItem::Image::Size::MEDIUM;
					case BandcampImage::Size::LARGE:
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




	Promise<Track::Data> BandcampProvider::getTrackData(String uri) {
		return bandcamp->getTrack(uri).map<Track::Data>([=](auto track) {
			return createTrackData(track);
		});
	}

	Promise<Artist::Data> BandcampProvider::getArtistData(String uri) {
		return bandcamp->getArtist(uri).map<Artist::Data>([=](auto artist) {
			return createArtistData(artist);
		});
	}

	Promise<Album::Data> BandcampProvider::getAlbumData(String uri) {
		return bandcamp->getAlbum(uri).map<Album::Data>([=](auto album) {
			return createAlbumData(album);
		});
	}

	Promise<Playlist::Data> BandcampProvider::getPlaylistData(String uri) {
		return Promise<Playlist::Data>::reject(std::logic_error("Bandcamp does not support playlists"));
	}



	MediaPlaybackProvider* BandcampProvider::player() {
		return nullptr;
	}

	const MediaPlaybackProvider* BandcampProvider::player() const {
		return nullptr;
	}
}

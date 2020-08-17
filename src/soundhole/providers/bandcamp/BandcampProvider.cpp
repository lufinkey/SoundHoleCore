//
//  BandcampProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "BandcampProvider.hpp"
#include "mutators/BandcampAlbumMutatorDelegate.hpp"
#include <cxxurl/url.hpp>

namespace sh {
	BandcampProvider::BandcampProvider()
	: bandcamp(new Bandcamp()), _player(new BandcampPlaybackProvider(this)) {
		//
	}

	BandcampProvider::~BandcampProvider() {
		delete _player;
		delete bandcamp;
	}

	String BandcampProvider::name() const {
		return "bandcamp";
	}

	String BandcampProvider::displayName() const {
		return "Bandcamp";
	}




	Bandcamp* BandcampProvider::api() {
		return bandcamp;
	}

	const Bandcamp* BandcampProvider::api() const {
		return bandcamp;
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




	Promise<BandcampProvider::SearchResults> BandcampProvider::search(String query, SearchOptions options) {
		return bandcamp->search(query, options).map<SearchResults>([=](BandcampSearchResults searchResults) -> SearchResults {
			return SearchResults{
				.prevURL=searchResults.prevURL,
				.nextURL=searchResults.nextURL,
				.items=searchResults.items.where([&](auto& item) {
					return (item.type != BandcampSearchResults::Item::Type::UNKNOWN);
				}).template map<$<MediaItem>>([&](auto& item) {
					auto images = (!item.imageURL.empty()) ?
						maybe(ArrayList<MediaItem::Image>{{
							.url=item.imageURL,
							.size=MediaItem::Image::Size::SMALL
						}})
						: std::nullopt;
					switch(item.type) {
						case BandcampSearchResults::Item::Type::TRACK:
							return std::static_pointer_cast<MediaItem>(Track::new$(this, Track::Data{{
								.partial=true,
								.type="track",
								.name=item.name,
								.uri=item.url,
								.images=images
								},
								.albumName=item.albumName,
								.albumURI=item.albumURL,
								.artists=ArrayList<$<Artist>>{
									Artist::new$(this, Artist::Data{{
										.partial=true,
										.type="artist",
										.name=item.artistName,
										.uri=item.artistURL,
										.images=std::nullopt
										},
										.description=std::nullopt
									})
								},
								.tags=item.tags,
								.discNumber=std::nullopt,
								.trackNumber=std::nullopt,
								.duration=std::nullopt,
								.audioSources=std::nullopt,
								.playable=true
							}));
							
						case BandcampSearchResults::Item::Type::ALBUM:
							return std::static_pointer_cast<MediaItem>(Album::new$(this, Album::Data{{{
								.partial=true,
								.type="album",
								.name=item.name,
								.uri=item.url,
								.images=images
								},
								.versionId=String(),
								.tracks=item.numTracks ?
									maybe(Album::Data::Tracks{
										.total=item.numTracks.value(),
										.offset=0,
										.items={}
									})
									: std::nullopt
								},
								.artists=ArrayList<$<Artist>>{
									Artist::new$(this, Artist::Data{{
										.partial=true,
										.type="artist",
										.name=item.artistName,
										.uri=item.artistURL,
										.images=std::nullopt
										},
										.description=std::nullopt
									})
								}
							}));
							
						case BandcampSearchResults::Item::Type::ARTIST:
							return std::static_pointer_cast<MediaItem>(Artist::new$(this, Artist::Data{{
								.partial=true,
								.type="artist",
								.name=item.name,
								.uri=item.url,
								.images=images
								},
								.description=std::nullopt
							}));
							
						case BandcampSearchResults::Item::Type::LABEL:
							return std::static_pointer_cast<MediaItem>(Artist::new$(this, Artist::Data{{
								.partial=true,
								.type="label",
								.name=item.name,
								.uri=item.url,
								.images=images
								},
								.description=std::nullopt
							}));
							
						case BandcampSearchResults::Item::Type::FAN:
							return std::static_pointer_cast<MediaItem>(UserAccount::new$(this, UserAccount::Data{{
								.partial=true,
								.type="user",
								.name=item.name,
								.uri=item.url,
								.images=images
								},
								.id=([&]() -> String {
									if(item.url.empty()) {
										return item.url;
									}
									try {
										auto url = Url(item.url);
										if(url.host() == "bandcamp.com" || url.host() == "www.bandcamp.com") {
											String id = url.path();
											while(id.startsWith("/")) {
												id = id.substring(1);
											}
											while(id.endsWith("/")) {
												id = id.substring(0, id.length()-1);
											}
											return id;
										}
										return item.url;
									} catch(Url::parse_error& error) {
										return item.url;
									}
								})(),
								.displayName=item.name
							}));
							
						case BandcampSearchResults::Item::Type::UNKNOWN:
							throw std::logic_error("found unknown bandcamp item");
					}
				})
			};
		});
	}




	Track::Data BandcampProvider::createTrackData(BandcampTrack track, bool partial) {
		return Track::Data{{
			.partial=partial,
			.type="track",
			.name=track.name,
			.uri=track.url,
			.images=track.images.map<MediaItem::Image>([&](auto& image) {
				return createImage(image);
			})
			},
			.albumName=track.albumName,
			.albumURI=track.albumURL,
			.artists=ArrayList<$<Artist>>{
				Artist::new$(this,
					track.artist ?
					createArtistData(track.artist.value(), partial)
					: Artist::Data{{
						.partial = true,
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
			.audioSources=(track.audioURL.has_value() ?
				maybe((!track.audioURL->empty()) ?
					ArrayList<Track::AudioSource>{
						Track::AudioSource{
							.url=track.audioURL.value(),
							.encoding="mp3",
							.bitrate=128.0
						}
					}
					: ArrayList<Track::AudioSource>{})
				: ((track.playable.has_value() && !track.playable.value()) ?
				   maybe(ArrayList<Track::AudioSource>{})
				   : std::nullopt
				)),
			.playable=track.playable.value_or(true)
		};
	}

	Artist::Data BandcampProvider::createArtistData(BandcampArtist artist, bool partial) {
		return Artist::Data{{
			.partial=partial,
			.type="artist",
			.name=artist.name,
			.uri=artist.url,
			.images=artist.images.map<MediaItem::Image>([&](auto& image) {
				return createImage(image);
			})
			},
			.description=artist.description
		};
	}

	Album::Data BandcampProvider::createAlbumData(BandcampAlbum album, bool partial) {
		auto artist = Artist::new$(this,
			album.artist ?
			createArtistData(album.artist.value(), partial)
			: Artist::Data{{
				.partial=true,
				.type="artist",
				.name=album.artistName,
				.uri=album.artistURL,
				.images=std::nullopt
				},
				.description=std::nullopt
			}
		);
		return Album::Data{{{
			.partial=partial,
			.type="album",
			.name=album.name,
			.uri=album.url,
			.images=album.images.map<MediaItem::Image>([&](auto& image) {
				return createImage(std::move(image));
			})
			},
			.versionId=String(),
			.tracks=(album.tracks ?
				maybe(Album::Data::Tracks{
					.total=album.tracks->size(),
					.offset=0,
					.items=album.tracks->map<AlbumItem::Data>([&](auto& track) {
						auto trackData = createTrackData(track, true);
						trackData.albumName = album.name;
						trackData.albumURI = album.url;
						if(trackData.artists.size() == 0) {
							trackData.artists = { artist };
						} else {
							for(size_t i=0; i<trackData.artists.size(); i++) {
								auto cmpArtist = trackData.artists[i];
								if(cmpArtist->uri() == artist->uri() && cmpArtist->name() == artist->name()) {
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
			return createTrackData(track, false);
		});
	}

	Promise<Artist::Data> BandcampProvider::getArtistData(String uri) {
		return bandcamp->getArtist(uri).map<Artist::Data>([=](auto artist) {
			return createArtistData(artist, false);
		});
	}

	Promise<Album::Data> BandcampProvider::getAlbumData(String uri) {
		return bandcamp->getAlbum(uri).map<Album::Data>([=](auto album) {
			return createAlbumData(album, false);
		});
	}

	Promise<Playlist::Data> BandcampProvider::getPlaylistData(String uri) {
		return Promise<Playlist::Data>::reject(std::logic_error("Bandcamp does not support playlists"));
	}

	Promise<UserAccount::Data> BandcampProvider::getUserData(String uri) {
		return Promise<UserAccount::Data>::reject(std::logic_error("This method is not implemented yet"));
	}




	Promise<ArrayList<$<Track>>> BandcampProvider::getArtistTopTracks(String artistURI) {
		return Promise<ArrayList<$<Track>>>::reject(std::runtime_error("Bandcamp does not have Top Tracks"));
	}

	Promise<std::tuple<$<Artist>,LinkedList<$<Album>>>> BandcampProvider::getArtistAndAlbums(String artistURI) {
		using ArtistAlbumsTuple = std::tuple<$<Artist>,LinkedList<$<Album>>>;
		return bandcamp->getArtist(artistURI).map<ArtistAlbumsTuple>([=](auto bcArtist) {
			if(!bcArtist.albums) {
				throw std::runtime_error("Failed to parse bandcamp artist's albums");
			}
			auto artist = Artist::new$(this, createArtistData(bcArtist, false));
			return ArtistAlbumsTuple{
				artist,
				bcArtist.albums->template map<$<Album>>([=](auto bcAlbum) {
					auto data = createAlbumData(bcAlbum, true);
					for(size_t i=0; i<data.artists.size(); i++) {
						auto cmpArtist = data.artists[i];
						if(cmpArtist->uri() == artist->uri()) {
							data.artists[i] = artist;
							break;
						}
					}
					return Album::new$(this,data);
				})
			};
		});
	}

	BandcampProvider::ArtistAlbumsGenerator BandcampProvider::getArtistAlbums(String artistURI) {
		using YieldResult = ArtistAlbumsGenerator::YieldResult;
		return ArtistAlbumsGenerator([=]() {
			return getArtistAndAlbums(artistURI).map<YieldResult>([=](auto tuple) {
				auto& albums = std::get<LinkedList<$<Album>>>(tuple);
				size_t total = albums.size();
				return YieldResult{
					.value=LoadBatch<$<Album>>{
						.items=std::move(albums),
						.total=total
					},
					.done=true
				};
			});
		});
	}

	BandcampProvider::UserPlaylistsGenerator BandcampProvider::getUserPlaylists(String userURI) {
		using YieldResult = typename UserPlaylistsGenerator::YieldResult;
		return UserPlaylistsGenerator([=]() {
			return Promise<YieldResult>::reject(std::runtime_error("Bandcamp does not have user playlists"));
		});
	}




	Album::MutatorDelegate* BandcampProvider::createAlbumMutatorDelegate($<Album> album) {
		return new BandcampAlbumMutatorDelegate(album);
	}

	Playlist::MutatorDelegate* BandcampProvider::createPlaylistMutatorDelegate($<Playlist> playlist) {
		throw std::logic_error("Bandcamp does not support playlists");
	}




	bool BandcampProvider::hasLibrary() const {
		return false;
	}

	BandcampProvider::LibraryItemGenerator BandcampProvider::generateLibrary(GenerateLibraryOptions options) {
		using YieldResult = typename LibraryItemGenerator::YieldResult;
		return LibraryItemGenerator([=]() {
			// TODO implement BandcampProvider::generateLibrary
			return Promise<YieldResult>::resolve(YieldResult{
				.done=true
			});
		});
	}




	BandcampPlaybackProvider* BandcampProvider::player() {
		return _player;
	}

	const BandcampPlaybackProvider* BandcampProvider::player() const {
		return _player;
	}
}

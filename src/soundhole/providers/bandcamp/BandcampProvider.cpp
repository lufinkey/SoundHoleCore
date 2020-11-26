//
//  BandcampProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "BandcampProvider.hpp"
#include "mutators/BandcampAlbumMutatorDelegate.hpp"
#include <soundhole/utils/SoundHoleError.hpp>
#include <soundhole/utils/Utils.hpp>
#include <cxxurl/url.hpp>
#include <regex>

namespace sh {
	BandcampProvider::BandcampProvider(Options options)
	: bandcamp(new Bandcamp(options)),
	_player(new BandcampPlaybackProvider(this)),
	_currentIdentitiesNeedRefresh(true) {
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



	#pragma mark URI/URL parsing

	BandcampProvider::URI BandcampProvider::parseURI(String uri) const {
		size_t colonIndex = uri.indexOf(':');
		if(colonIndex == (size_t)-1) {
			throw SoundHoleError(SoundHoleError::Code::PARSE_FAILED, "invalid bandcamp uri "+uri);
		}
		// validate provider name
		String providerName = uri.substring(0, colonIndex);
		if(providerName != name()) {
			throw SoundHoleError(SoundHoleError::Code::PARSE_FAILED, "invalid bandcamp uri "+uri);
		}
		// validate url
		String urlString = uri.substring(colonIndex+1);
		Url(urlString).str();
		return URI{
			.provider=providerName,
			.url=urlString
		};
	}

	BandcampProvider::URI BandcampProvider::parseURL(String url) const {
		// validate url
		Url(url).str();
		return URI{
			.provider=name(),
			.url=url
		};
	}

	String BandcampProvider::createURI(String type, String url) const {
		if(url.empty()) {
			throw std::logic_error("url cannot be empty in bandcamp uri");
		}
		return name()+':'+url;
	}



	#pragma mark Login

	Promise<bool> BandcampProvider::login() {
		return bandcamp->login().map<bool>([=](bool loggedIn) {
			getCurrentBandcampIdentities();
			return loggedIn;
		});
	}

	void BandcampProvider::logout() {
		bandcamp->logout();
		setCurrentBandcampIdentities(std::nullopt);
	}

	bool BandcampProvider::isLoggedIn() const {
		return bandcamp->isLoggedIn();
	}



	#pragma mark Current User

	Promise<ArrayList<String>> BandcampProvider::getCurrentUserIds() {
		return getCurrentBandcampIdentities().map<ArrayList<String>>([=](Optional<BandcampIdentities> identities) -> ArrayList<String> {
			if(!identities || !identities->fan) {
				return {};
			}
			return { identities->fan->id };
		});
	}

	String BandcampProvider::getCachedCurrentBandcampIdentitiesPath() const {
		String sessionPersistKey = bandcamp->getAuth()->getOptions().sessionPersistKey;
		if(sessionPersistKey.empty()) {
			return String();
		}
		return utils::getCacheDirectoryPath()+"/"+name()+"_identities_"+sessionPersistKey+".json";
	}

	Promise<Optional<BandcampIdentities>> BandcampProvider::getCurrentBandcampIdentities() {
		if(!isLoggedIn()) {
			setCurrentBandcampIdentities(std::nullopt);
			return Promise<Optional<BandcampIdentities>>::resolve(std::nullopt);
		}
		if(_currentIdentitiesPromise) {
			return _currentIdentitiesPromise.value();
		}
		if(_currentIdentities && !_currentIdentitiesNeedRefresh) {
			return Promise<Optional<BandcampIdentities>>::resolve(_currentIdentities);
		}
		auto promise = bandcamp->getMyIdentities().map<Optional<BandcampIdentities>>([=](BandcampIdentities identities) -> Optional<BandcampIdentities> {
			_currentIdentitiesPromise = std::nullopt;
			if(!isLoggedIn()) {
				setCurrentBandcampIdentities(std::nullopt);
				return std::nullopt;
			}
			setCurrentBandcampIdentities(identities);
			return identities;
		}).except([=](std::exception_ptr error) -> Optional<BandcampIdentities> {
			_currentIdentitiesPromise = std::nullopt;
			// if current identities are loaded, return it
			if(_currentIdentities) {
				return _currentIdentities.value();
			}
			// try to load identities from filesystem
			auto userFilePath = getCachedCurrentBandcampIdentitiesPath();
			if(!userFilePath.empty()) {
				try {
					String userString = fs::readFile(userFilePath);
					std::string jsonError;
					Json userJson = Json::parse(userString, jsonError);
					if(userJson.is_null()) {
						throw std::runtime_error("failed to parse user json: "+jsonError);
					}
					auto identities = BandcampIdentities::fromJson(userJson);
					_currentIdentities = identities;
					_currentIdentitiesNeedRefresh = true;
					return identities;
				} catch(...) {
					// ignore error
				}
			}
			// rethrow error
			std::rethrow_exception(error);
		});
		_currentIdentitiesPromise = promise;
		if(_currentIdentitiesPromise && _currentIdentitiesPromise->isComplete()) {
			_currentIdentitiesPromise = std::nullopt;
		}
		return promise;
	}

	void BandcampProvider::setCurrentBandcampIdentities(Optional<BandcampIdentities> identities) {
		auto userFilePath = getCachedCurrentBandcampIdentitiesPath();
		if(identities) {
			_currentIdentities = identities;
			_currentIdentitiesNeedRefresh = false;
			if(!userFilePath.empty()) {
				fs::writeFile(userFilePath, identities->toJson().dump());
			}
		} else {
			_currentIdentities = std::nullopt;
			_currentIdentitiesPromise = std::nullopt;
			_currentIdentitiesNeedRefresh = true;
			if(!userFilePath.empty() && fs::exists(userFilePath)) {
				fs::remove(userFilePath);
			}
		}
	}



	#pragma mark Search

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
								.uri=(item.url.empty() ? String() : createURI("track", item.url)),
								.images=images
								},
								.albumName=item.albumName,
								.albumURI=(item.albumURL.empty() ? String() : createURI("album", item.albumURL)),
								.artists=ArrayList<$<Artist>>{
									Artist::new$(this, Artist::Data{{
										.partial=true,
										.type="artist",
										.name=item.artistName,
										.uri=(item.artistURL.empty() ? String() : createURI("artist", item.artistURL)),
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
								.uri=(item.url.empty() ? String() : createURI("album", item.url)),
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
										.uri=(item.artistURL.empty() ? String() : createURI("artist", item.artistURL)),
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
								.uri=(item.url.empty() ? String() : createURI("artist", item.url)),
								.images=images
								},
								.description=std::nullopt
							}));
							
						case BandcampSearchResults::Item::Type::LABEL:
							return std::static_pointer_cast<MediaItem>(Artist::new$(this, Artist::Data{{
								.partial=true,
								.type="artist",
								.name=item.name,
								.uri=(item.url.empty() ? String() : createURI("artist", item.url)),
								.images=images
								},
								.description=std::nullopt
							}));
							
						case BandcampSearchResults::Item::Type::FAN:
							return std::static_pointer_cast<MediaItem>(UserAccount::new$(this, UserAccount::Data{{
								.partial=true,
								.type="user",
								.name=item.name,
								.uri=(item.url.empty() ? String() : createURI("user", item.url)),
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



	#pragma mark Data transforming

	Track::Data BandcampProvider::createTrackData(BandcampTrack track, bool partial) {
		return Track::Data{{
			.partial=partial,
			.type="track",
			.name=track.name,
			.uri=(track.url.empty() ? String() : createURI("track", track.url)),
			.images=track.images.map<MediaItem::Image>([&](auto& image) {
				return createImage(image);
			})
			},
			.albumName=track.albumName,
			.albumURI=(track.albumURL.empty() ? String() : createURI("album", track.albumURL)),
			.artists=ArrayList<$<Artist>>{
				Artist::new$(this,
					track.artist ?
					createArtistData(track.artist.value(), partial)
					: Artist::Data{{
						.partial = true,
						.type="artist",
						.name=track.artistName,
						.uri=(track.artistURL.empty() ? String() : createURI("artist", track.artistURL)),
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
			.audioSources=(track.audioSources.has_value() ?
				maybe(track.audioSources->map<Track::AudioSource>([](auto& audioSource) {
					String encoding = "mp3";
					double bitrate = 128.0;
					std::cmatch matches;
					std::regex expr("^([\\w\\-]+)\\-([\\d\\.]+)$");
					if(std::regex_match(audioSource.type.c_str(), matches, expr)) {
						encoding = matches[1].str();
						bitrate = String(matches[2].str()).toArithmeticValue<double>();
					}
					return Track::AudioSource{
						.url=audioSource.url,
						.encoding=encoding,
						.bitrate=bitrate
					};
				}))
				: ((track.playable.has_value() && !track.playable.value()) ?
				   maybe(ArrayList<Track::AudioSource>{})
				   : std::nullopt)
				),
			.playable=track.playable.value_or(track.audioSources ? (track.audioSources->size() > 0) : true)
		};
	}

	Artist::Data BandcampProvider::createArtistData(BandcampArtist artist, bool partial) {
		return Artist::Data{{
			.partial=partial,
			.type="artist",
			.name=artist.name,
			.uri=(artist.url.empty() ? String() : createURI("artist", artist.url)),
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
				.uri=(album.artistURL.empty() ? String() : createURI("artist", album.artistURL)),
				.images=std::nullopt
				},
				.description=std::nullopt
			}
		);
		return Album::Data{{{
			.partial=partial,
			.type="album",
			.name=album.name,
			.uri=(album.url.empty() ? String() : createURI("album", album.url)),
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

	UserAccount::Data BandcampProvider::createUserData(BandcampFan fan) {
		return UserAccount::Data{{
			.partial=false,
			.type="user",
			.name=fan.name,
			.uri=(fan.url.empty() ? String() : createURI("fan", fan.url)),
			.images=(fan.images ? maybe(fan.images->map<MediaItem::Image>([&](auto& image) {
				return createImage(std::move(image));
			})) : std::nullopt)
			},
			.id=fan.id,
			.displayName=fan.name
		};
	}



	#pragma mark Media Item Fetching

	Promise<Track::Data> BandcampProvider::getTrackData(String uri) {
		auto uriParts = parseURI(uri);
		return bandcamp->getTrack(uriParts.url).map<Track::Data>([=](auto track) {
			return createTrackData(track, false);
		});
	}

	Promise<Artist::Data> BandcampProvider::getArtistData(String uri) {
		auto uriParts = parseURI(uri);
		return bandcamp->getArtist(uriParts.url).map<Artist::Data>([=](auto artist) {
			return createArtistData(artist, false);
		});
	}

	Promise<Album::Data> BandcampProvider::getAlbumData(String uri) {
		auto uriParts = parseURI(uri);
		return bandcamp->getAlbum(uriParts.url).map<Album::Data>([=](auto album) {
			return createAlbumData(album, false);
		});
	}

	Promise<Playlist::Data> BandcampProvider::getPlaylistData(String uri) {
		return Promise<Playlist::Data>::reject(std::logic_error("Bandcamp does not support playlists"));
	}

	Promise<UserAccount::Data> BandcampProvider::getUserData(String uri) {
		auto uriParts = parseURI(uri);
		return bandcamp->getFan(uriParts.url).map<UserAccount::Data>([=](auto fan) {
			return createUserData(fan);
		});
	}




	Promise<ArrayList<$<Track>>> BandcampProvider::getArtistTopTracks(String artistURI) {
		return Promise<ArrayList<$<Track>>>::reject(std::runtime_error("Bandcamp does not have Top Tracks"));
	}

	Promise<std::tuple<$<Artist>,LinkedList<$<Album>>>> BandcampProvider::getArtistAndAlbums(String artistURI) {
		using ArtistAlbumsTuple = std::tuple<$<Artist>,LinkedList<$<Album>>>;
		auto uriParts = parseURI(artistURI);
		return bandcamp->getArtist(uriParts.url).map<ArtistAlbumsTuple>([=](auto bcArtist) {
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
		auto uriParts = parseURI(artistURI);
		return ArtistAlbumsGenerator([=]() {
			return getArtistAndAlbums(uriParts.url).map<YieldResult>([=](auto tuple) {
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



	#pragma mark User Library

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



	#pragma mark Player

	BandcampPlaybackProvider* BandcampProvider::player() {
		return _player;
	}

	const BandcampPlaybackProvider* BandcampProvider::player() const {
		return _player;
	}
}

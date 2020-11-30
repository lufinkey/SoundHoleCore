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



	time_t BandcampProvider::timeFromString(String time) {
		if(auto date = DateTime::fromGmtString(time, "%Y-%m-%dT%H:%M:%SZ")) {
			return date->toTimeType();
		}
		throw std::invalid_argument("Invalid date format");
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

	ArrayList<$<Artist>> BandcampProvider::createArtists(String artistURL, String artistName, Optional<BandcampArtist> artist, bool partial) {
		ArrayList<$<Artist>> artists;
		if(artist && !artistURL.empty() && artist->url == artistURL && artist->name != artistName) {
			artistURL = String();
		}
		if(!artist || (artistURL.empty() && !artistName.empty() && artist->name != artistName) || (!artistURL.empty() && artistURL != artist->url)) {
			artists.pushBack(Artist::new$(this,Artist::Data{{
				.partial = true,
				.type="artist",
				.name=artistName,
				.uri=(artistURL.empty() ? String() : createURI("artist", artistURL)),
				.images=std::nullopt
				},
				.description=std::nullopt
			}));
		}
		if(artist) {
			artists.pushBack(Artist::new$(this,createArtistData(artist.value(), partial)));
		}
		return artists;
	}

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
			.artists=createArtists(track.artistURL, track.artistName, track.artist, partial),
			.tags=track.tags,
			.discNumber=std::nullopt,
			.trackNumber=track.trackNumber,
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

	Track::Data BandcampProvider::createTrackData(BandcampFan::CollectionTrack track) {
		return Track::Data{{
			.partial=true,
			.type="track",
			.name=track.name,
			.uri=(track.url.empty() ? String() : createURI("track", track.url)),
			.images=(track.images ? maybe(track.images->map<MediaItem::Image>([&](auto& image) {
				return createImage(image);
			})) : std::nullopt)
			},
			.albumName=track.albumName.valueOr(track.albumSlug.valueOr(String())),
			.albumURI=(track.albumURL.empty() ? String() : createURI("album", track.albumURL)),
			.artists=createArtists(track.artistURL, track.artistName, std::nullopt, true),
			.tags=std::nullopt,
			.discNumber=std::nullopt,
			.trackNumber=track.trackNumber,
			.duration=track.duration,
			.audioSources=std::nullopt,
			.playable=true
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

	Artist::Data BandcampProvider::createArtistData(BandcampFan::CollectionArtist artist) {
		return Artist::Data{{
			.partial=true,
			.type="artist",
			.name=artist.name,
			.uri=(artist.url.empty() ? String() : createURI("artist", artist.url)),
			.images=artist.images ? maybe(artist.images->map<MediaItem::Image>([&](auto& image) {
				return createImage(image);
			})) : std::nullopt
			},
			.description=std::nullopt
		};
	}

	Album::Data BandcampProvider::createAlbumData(BandcampAlbum album, bool partial) {
		auto artists = createArtists(album.artistURL, album.artistName, album.artist, partial);
		auto albumURI = (album.url.empty() ? String() : createURI("album", album.url));
		return Album::Data{{{
			.partial=partial,
			.type="album",
			.name=album.name,
			.uri=albumURI,
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
						auto trackData = createTrackData(track, partial);
						trackData.albumName = album.name;
						trackData.albumURI = albumURI;
						// pull duplicate artists from album artists
						if(trackData.artists.size() == 0) {
							trackData.artists = artists;
						} else {
							for(auto& artist : trackData.artists) {
								if(artist->uri().empty()) {
									continue;
								}
								if(auto matchingArtist = artists.firstWhere([&](auto& item){return (item->uri() == artist->uri());}, nullptr)) {
									if(artist->name() == matchingArtist->name()) {
										artist = matchingArtist;
									}
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
			.artists=artists,
		};
	}

	Album::Data BandcampProvider::createAlbumData(BandcampFan::CollectionAlbum album) {
		return Album::Data{{{
			.partial=true,
			.type="album",
			.name=album.name,
			.uri=(album.url.empty() ? String() : createURI("album", album.url)),
			.images=album.images ? maybe(album.images->map<MediaItem::Image>([&](auto& image) {
				return createImage(image);
			})) : std::nullopt
			},
			.versionId=String(),
			.tracks=std::nullopt,
			},
			.artists=createArtists(album.artistURL, album.artistName, std::nullopt, true)
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

	UserAccount::Data BandcampProvider::createUserData(BandcampFan::CollectionFan fan) {
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

	BandcampProvider::GenerateLibraryResumeData BandcampProvider::GenerateLibraryResumeData::fromJson(const Json& json) {
		auto mostRecentHiddenSave = json["mostRecentHiddenSave"];
		auto mostRecentCollectionSave = json["mostRecentCollectionSave"];
		auto mostRecentWishlistSave = json["mostRecentWishlistSave"];
		auto syncMostRecentSave = json["syncMostRecentSave"];
		auto resumeData = GenerateLibraryResumeData{
			.fanId = (std::string)json["fanId"].string_value(),
			.mostRecentHiddenSave = mostRecentHiddenSave.is_number() ? maybe((time_t)mostRecentHiddenSave.number_value()) : std::nullopt,
			.mostRecentCollectionSave = mostRecentCollectionSave.is_number() ? maybe((time_t)mostRecentCollectionSave.number_value()) : std::nullopt,
			.mostRecentWishlistSave = mostRecentWishlistSave.is_number() ? maybe((time_t)mostRecentWishlistSave.number_value()) : std::nullopt,
			.syncCurrentType = json["syncCurrentType"].string_value(),
			.syncMostRecentSave = syncMostRecentSave.is_number() ? maybe((time_t)syncMostRecentSave.number_value()) : std::nullopt,
			.syncLastToken = json["syncLastToken"].string_value(),
			.syncOffset = (size_t)json["syncOffset"].number_value()
		};
		bool syncTypeValid = ArrayList<String>{"hidden","collection","wishlist"}.contains(resumeData.syncCurrentType);
		if(!(resumeData.syncMostRecentSave.has_value() && !resumeData.syncLastToken.empty()
			 && syncTypeValid)) {
			if(!syncTypeValid) {
				resumeData.syncCurrentType = String();
			}
			resumeData.syncMostRecentSave = std::nullopt;
			resumeData.syncLastToken = String();
			resumeData.syncOffset = 0;
		}
		return resumeData;
	}

	Json BandcampProvider::GenerateLibraryResumeData::toJson() const {
		return Json::object{
			{ "fanId", (std::string)fanId },
			{ "mostRecentHiddenSave", mostRecentHiddenSave ? Json((double)mostRecentHiddenSave.value()) : Json() },
			{ "mostRecentCollectionSave", mostRecentCollectionSave ? Json((double)mostRecentCollectionSave.value()) : Json() },
			{ "mostRecentWishlistSave", mostRecentWishlistSave ? Json((double)mostRecentWishlistSave.value()) : Json() },
			{ "syncCurrentType", Json(syncCurrentType) },
			{ "syncMostRecentSave", syncMostRecentSave ? Json((double)syncMostRecentSave.value()) : Json() },
			{ "syncLastToken", (std::string)syncLastToken },
			{ "syncOffset", (double)syncOffset }
		};
	}

	String BandcampProvider::GenerateLibraryResumeData::typeFromSyncIndex(size_t index) {
		switch(index) {
			case 0:
				return "hidden";
			case 1:
				return "collection";
			case 2:
				return "wishlist";
		}
		return "";
	}

	Optional<size_t> BandcampProvider::GenerateLibraryResumeData::syncIndexFromType(String type) {
		if(type == "hidden") {
			return 0;
		} else if(type == "collection") {
			return 1;
		} else if(type == "wishlist") {
			return 2;
		}
		return std::nullopt;
	}

	bool BandcampProvider::hasLibrary() const {
		return true;
	}

	BandcampProvider::LibraryItemGenerator BandcampProvider::generateLibrary(GenerateLibraryOptions options) {
		auto resumeData = GenerateLibraryResumeData::fromJson(options.resumeData);
		struct SharedData {
			String fanId;
			Optional<BandcampFan> fan;
			double pendingItemsProgress = 0.0;
			double pendingItemProgressDiff = 0.0;
			bool pendingItemsFinishes = false;
			LinkedList<LibraryItem> pendingItems;
			Json pendingItemsResumeData;
			size_t pendingItemsType = 0;
			
			Optional<time_t> mostRecentHiddenSave;
			Optional<time_t> mostRecentCollectionSave;
			Optional<time_t> mostRecentWishlistSave;
			Optional<time_t> syncMostRecentSave;
			String lastToken;
			size_t type = 0;
			size_t offset = 0;
		};
		
		auto sharedData = fgl::new$<SharedData>(SharedData{
			.fanId = resumeData.fanId,
			.mostRecentHiddenSave = resumeData.mostRecentHiddenSave,
			.mostRecentCollectionSave = resumeData.mostRecentCollectionSave,
			.mostRecentWishlistSave = resumeData.mostRecentWishlistSave,
			.syncMostRecentSave = resumeData.syncMostRecentSave,
			.lastToken = resumeData.syncLastToken,
			.type = GenerateLibraryResumeData::syncIndexFromType(resumeData.syncCurrentType).value_or(0),
			.offset = resumeData.syncOffset
		});
		auto createResumeData = Function<GenerateLibraryResumeData($<SharedData>)>([]($<SharedData> data) {
			return GenerateLibraryResumeData{
				.mostRecentHiddenSave = data->mostRecentHiddenSave,
				.mostRecentCollectionSave = data->mostRecentCollectionSave,
				.mostRecentWishlistSave = data->mostRecentWishlistSave,
				.syncCurrentType = GenerateLibraryResumeData::typeFromSyncIndex(data->type),
				.syncMostRecentSave = data->syncMostRecentSave,
				.syncLastToken = data->lastToken,
				.syncOffset = data->offset
			};
		});
		
		using YieldResult = typename LibraryItemGenerator::YieldResult;
		return LibraryItemGenerator([=]() {
			return bandcamp->getMyIdentities().then([=](Optional<BandcampIdentities> identities) {
				// verify that bandcamp is logged in
				if(!identities) {
					if(!bandcamp->isLoggedIn()) {
						return Promise<YieldResult>::reject(std::runtime_error("Bandcamp is not logged in"));
					} else {
						return Promise<YieldResult>::reject(std::runtime_error("Bandcamp is logged in, but does not have any identities"));
					}
				} else if(!identities->fan) {
					return Promise<YieldResult>::reject(std::runtime_error("Bandcamp is logged in, but does not have a fan identity"));
				}
				// if current user has changed, completely reset the sync data
				if(sharedData->fanId.empty() || sharedData->fanId != identities->fan->id) {
					*sharedData = SharedData();
					sharedData->fanId = identities->fan->id;
					sharedData->fan = std::nullopt;
				}
				// fetch fan page data if needed
				auto promise = Promise<void>::resolve();
				if(!sharedData->fan) {
					promise = promise.then([=]() {
						return bandcamp->getFan(identities->fan->url).then([=](BandcampFan fan) {
							sharedData->fan = fan;
						});
					});
				}
				
				// dequeue pending library items function
				auto dequeuePendingItems = Function<Promise<YieldResult>(bool)>([=](bool saveAlbumTracks) {
					if(sharedData->pendingItems.size() == 0) {
						return Promise<YieldResult>::resolve(YieldResult{
							.value = GenerateLibraryResults{
								.resumeData = createResumeData(sharedData).toJson(),
								.items = {},
								.progress = sharedData->pendingItemsProgress
							},
							.done = sharedData->pendingItemsFinishes
						});
					}
					size_t maxItemsCount = 6;
					size_t pendingItemCount = 0;
					auto dequeuedItems = fgl::new$<LinkedList<LibraryItem>>();
					auto dequeuedIts = LinkedList<LinkedList<LibraryItem>::iterator>();
					auto promise = Promise<void>::resolve();
					double progress = sharedData->pendingItemsProgress;
					for(auto it=sharedData->pendingItems.begin(); it!=sharedData->pendingItems.end(); it++) {
						auto libraryItem = *it;
						dequeuedIts.pushBack(it);
						dequeuedItems->pushBack(libraryItem);
						pendingItemCount++;
						// fetch missing item data if needed
						if(libraryItem.mediaItem->needsData()) {
							promise = promise.then([=]() {
								// append album tracks if needed
								if(auto collection = std::dynamic_pointer_cast<TrackCollection>(libraryItem.mediaItem); saveAlbumTracks) {
									return libraryItem.mediaItem->fetchDataIfNeeded().then([=]() {
										if(!collection->itemCount().hasValue()) {
											throw std::runtime_error("bandcamp collection has no item count after fetching data");
										}
										for(size_t i=0; i<collection->itemCount().value(); i++) {
											auto item = collection->itemAt(i);
											if(!item) {
												throw std::runtime_error("missing track "+std::to_string(i+1)+" for "+collection->type()+" "+collection->name()+" after fetching data");
											}
											dequeuedItems->pushBack(LibraryItem{
												.libraryProvider = collection->mediaProvider(),
												.mediaItem = item->track(),
												.addedAt = libraryItem.addedAt
											});
										}
									});
								} else {
									return libraryItem.mediaItem->fetchDataIfNeeded();
								}
							});
						} else {
							// append album tracks if needed
							if(auto collection = std::dynamic_pointer_cast<TrackCollection>(libraryItem.mediaItem); saveAlbumTracks) {
								if(!collection->itemCount().hasValue()) {
									throw std::runtime_error("bandcamp collection has no item count");
								}
								for(size_t i=0; i<collection->itemCount().value(); i++) {
									auto item = collection->itemAt(i);
									if(!item) {
										throw std::runtime_error("missing track "+std::to_string(i+1)+" for "+collection->type()+" "+collection->name());
									}
									dequeuedItems->pushBack(LibraryItem{
										.libraryProvider = collection->mediaProvider(),
										.mediaItem = item->track(),
										.addedAt = libraryItem.addedAt
									});
								}
							}
						}
						// increment progress
						progress += sharedData->pendingItemProgressDiff;
						// stop if we have enough items
						if(pendingItemCount >= maxItemsCount || (saveAlbumTracks && std::dynamic_pointer_cast<TrackCollection>(libraryItem.mediaItem) != nullptr)) {
							break;
						}
					}
					bool dequeuedAll = (pendingItemCount >= sharedData->pendingItems.size());
					return promise.map<YieldResult>([=]() {
						// delete items from pendingItems
						for(auto& it : dequeuedIts) {
							sharedData->pendingItems.erase(it);
						}
						
						// finish
						if(dequeuedAll) {
							bool done = sharedData->pendingItemsFinishes;
							sharedData->pendingItemsType = 0;
							sharedData->pendingItemsProgress = 0.0;
							sharedData->pendingItemProgressDiff = 0.0;
							sharedData->pendingItemsResumeData = Json();
							sharedData->pendingItemsFinishes = false;
							sharedData->pendingItems = {};
							return YieldResult{
								.value=GenerateLibraryResults{
									.resumeData=createResumeData(sharedData).toJson(),
									.items=*dequeuedItems,
									.progress=progress
								},
								.done=done
							};
						} else {
							sharedData->pendingItemsProgress = progress;
							return YieldResult{
								.value=GenerateLibraryResults{
									.resumeData=sharedData->pendingItemsResumeData,
									.items=*dequeuedItems,
									.progress=progress
								},
								.done=false
							};
						}
					});
				});
				
				// generate library items
				return promise.then([=]() {
					// generate media items function
					size_t sectionCount = 3;
					auto generateSectionMediaItems = [=](const Optional<BandcampFan::Section<BandcampFan::CollectionItemNode>>& collection, Optional<time_t>* mostRecentSave, bool saveAlbumTracks, Function<Promise<BandcampFanSectionPage<BandcampFan::CollectionItemNode>>()> fetcher) -> Promise<YieldResult> {
						// if we have pending items, dequeue them
						if(sharedData->pendingItems.size() > 0) {
							return dequeuePendingItems(saveAlbumTracks);
						}
						
						// helper functions
						auto checkIndexSectionCaughtUp = [](const LinkedList<LibraryItem>& items, Optional<time_t> mostRecentSave) -> Optional<size_t> {
							if(!mostRecentSave) {
								return std::nullopt;
							}
							size_t index = 0;
							for(auto& item : items) {
								time_t addedAt = item.addedAt.empty() ? 0 : timeFromString(item.addedAt);
								if(addedAt < mostRecentSave.value()) {
									return index;
								}
								index++;
							}
							return std::nullopt;
						};
						auto mapLibraryItems = [](BandcampProvider* provider, const ArrayList<BandcampFan::CollectionItemNode>& items) -> LinkedList<LibraryItem> {
							LinkedList<LibraryItem> libraryItems;
							for(auto& item : items) {
								if(auto track = item.trackItem()) {
									libraryItems.pushBack(LibraryItem{
										.libraryProvider = provider,
										.mediaItem = Track::new$(provider, provider->createTrackData(track.value())),
										.addedAt = item.dateAdded
									});
								} else if(auto album = item.albumItem()) {
									libraryItems.pushBack(LibraryItem{
										.libraryProvider = provider,
										.mediaItem = Album::new$(provider, provider->createAlbumData(album.value())),
										.addedAt = item.dateAdded
									});
								}
							}
							return libraryItems;
						};
						
						if(!collection) {
							// collection is empty, so move to next section
							bool done = false;
							*mostRecentSave = std::nullopt;
							sharedData->type += 1;
							sharedData->offset = 0;
							double progress = (double)sharedData->type / (double)sectionCount;
							if(sharedData->type >= sectionCount) {
								sharedData->type = 0;
								progress = 1.0;
								done = true;
							}
							sharedData->lastToken = String();
							sharedData->syncMostRecentSave = std::nullopt;
							return Promise<YieldResult>::resolve(YieldResult{
								.value=GenerateLibraryResults{
									.resumeData = createResumeData(sharedData).toJson(),
									.items = {},
									.progress = progress
								},
								.done=done
							});
						}
						// if we don't have a last token, get the items from the fan page and set the last token
						if(sharedData->lastToken.empty()) {
							// save old resume data and progress
							auto oldResumeData = createResumeData(sharedData).toJson();
							double oldSectionProgress = (double)sharedData->offset / (double)collection->itemCount;
							if(oldSectionProgress > 1.0) {
								oldSectionProgress = 1.0;
							}
							double oldProgress = ((double)sharedData->type + oldSectionProgress) / (double)sectionCount;
							size_t oldType = sharedData->type;
							// map items
							auto items = mapLibraryItems(this, collection->items);
							// update last token, offset, and initial item save
							if(collection->items.size() > 0) {
								sharedData->syncMostRecentSave = timeFromString(collection->items.front().dateAdded);
							} else {
								sharedData->syncMostRecentSave = std::nullopt;
							}
							sharedData->lastToken = collection->lastToken;
							sharedData->offset += collection->items.size();
							double sectionProgress = (double)sharedData->offset / (double)collection->itemCount;
							if(sectionProgress > 1.0) {
								sectionProgress = 1.0;
							}
							double progress = ((double)sharedData->type + sectionProgress) / (double)sectionCount;
							// check if syncing is caught up or finished
							bool done = false;
							auto caughtUpIndex = checkIndexSectionCaughtUp(items, *mostRecentSave);
							if(caughtUpIndex
							   || collection->itemCount <= collection->items.size()) {
								// trim items
								if(caughtUpIndex) {
									items = LinkedList<LibraryItem>(items.begin(), std::next(items.begin(),caughtUpIndex.value()));
								}
								// move to next section
								*mostRecentSave = sharedData->syncMostRecentSave;
								sharedData->type += 1;
								progress = ((double)sharedData->type / (double)sectionCount);
								if(sharedData->type >= sectionCount) {
									sharedData->type = 0;
									progress = 1.0;
									done = true;
								}
								sharedData->offset = 0;
								sharedData->lastToken = String();
								sharedData->syncMostRecentSave = std::nullopt;
							}
							// if we don't have any items, return what we have
							if(items.size() == 0) {
								// return fan page items
								return Promise<YieldResult>::resolve(YieldResult{
									.value=GenerateLibraryResults{
										.resumeData = createResumeData(sharedData).toJson(),
										.items = items,
										.progress = progress
									},
									.done=done
								});
							}
							// set pending items
							sharedData->pendingItems = items;
							sharedData->pendingItemsType = oldType;
							sharedData->pendingItemsProgress = oldProgress;
							sharedData->pendingItemProgressDiff = (progress - oldProgress) / items.size();
							sharedData->pendingItemsResumeData = oldResumeData;
							sharedData->pendingItemsFinishes = done;
							// dequeue pending items
							return dequeuePendingItems(saveAlbumTracks);
						}
						// fetch items
						return fetcher().then([=](BandcampFanSectionPage<BandcampFan::CollectionItemNode> page) {
							// ensure we have a token for the next page
							if(page.hasMore && page.lastToken.empty()) {
								return Promise<YieldResult>::reject(std::runtime_error("missing lastToken for bandcamp library page"));
							}
							// save old resume data and progress
							auto oldResumeData = createResumeData(sharedData).toJson();
							double oldSectionProgress = (double)sharedData->offset / (double)collection->itemCount;
							if(oldSectionProgress > 1.0) {
								oldSectionProgress = 1.0;
							}
							double oldProgress = ((double)sharedData->type + oldSectionProgress) / (double)sectionCount;
							size_t oldType = sharedData->type;
							// map items and increment offset
							auto items = mapLibraryItems(this, page.items);
							sharedData->offset += items.size();
							// check if sync is finished
							bool done = false;
							double sectionProgress = (double)sharedData->offset / (double)collection->itemCount;
							if(sectionProgress > 1.0) {
								sectionProgress = 1.0;
							}
							double progress = ((double)sharedData->type + sectionProgress) / (double)sectionCount;
							auto caughtUpIndex = checkIndexSectionCaughtUp(items, *mostRecentSave);
							if(caughtUpIndex || !page.hasMore || page.lastToken.empty()) {
								// trim items
								if(caughtUpIndex) {
									items = LinkedList<LibraryItem>(items.begin(), std::next(items.begin(),caughtUpIndex.value()));
								}
								// move to next section
								*mostRecentSave = sharedData->syncMostRecentSave;
								sharedData->type += 1;
								if(sharedData->type >= sectionCount) {
									sharedData->type = 0;
									progress = 1.0;
									done = true;
								} else {
									progress = ((double)sharedData->type / sectionCount);
								}
								sharedData->offset = 0;
								sharedData->lastToken = String();
								sharedData->syncMostRecentSave = std::nullopt;
							} else {
								sharedData->lastToken = page.lastToken;
							}
							// if we don't have any items, return what we have
							if(items.size() == 0) {
								return Promise<YieldResult>::resolve(YieldResult{
									.value=GenerateLibraryResults{
										.resumeData = createResumeData(sharedData).toJson(),
										.items = items,
										.progress = progress
									},
									done = done
								});
							}
							// set pending items
							sharedData->pendingItems = items;
							sharedData->pendingItemsType = oldType;
							sharedData->pendingItemsProgress = oldProgress;
							sharedData->pendingItemProgressDiff = (progress - oldProgress) / items.size();
							sharedData->pendingItemsResumeData = oldResumeData;
							sharedData->pendingItemsFinishes = done;
							// dequeue pending items
							return dequeuePendingItems(saveAlbumTracks);
						});
					};
					
					// check section
					size_t type = sharedData->type;
					if(sharedData->pendingItems.size() > 0) {
						type = sharedData->pendingItemsType;
					}
					switch(type) {
						// hidden items
						case 0: {
							return generateSectionMediaItems(sharedData->fan->hiddenCollection, &sharedData->mostRecentHiddenSave, true, [=]() {
								return bandcamp->getFanHiddenItems(sharedData->fan->url, sharedData->fanId, {
									.olderThanToken = sharedData->lastToken,
									.count = sharedData->fan->hiddenCollection->batchSize.valueOr(20)
								});
							});
						}
						
						// collection items
						case 1: {
							return generateSectionMediaItems(sharedData->fan->collection, &sharedData->mostRecentCollectionSave, true, [=]() {
								return bandcamp->getFanCollectionItems(sharedData->fan->url, sharedData->fanId, {
									.olderThanToken = sharedData->lastToken,
									.count = sharedData->fan->collection->batchSize.valueOr(20)
								});
							});
						}
						
						// wishlist items
						case 2: {
							return generateSectionMediaItems(sharedData->fan->wishlist, &sharedData->mostRecentWishlistSave, false, [=]() {
								return bandcamp->getFanWishlistItems(sharedData->fan->url, sharedData->fanId, {
									.olderThanToken = sharedData->lastToken,
									.count = sharedData->fan->wishlist->batchSize.valueOr(20)
								});
							});
						}
					}
					return Promise<YieldResult>::reject(std::runtime_error("invalid bandcamp sync section"));
				});
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

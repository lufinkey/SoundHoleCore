//
//  BandcampMediaProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "BandcampMediaProvider.hpp"
#include "mutators/BandcampAlbumMutatorDelegate.hpp"
#include <soundhole/utils/SoundHoleError.hpp>
#include <soundhole/utils/Utils.hpp>
#include <regex>

namespace sh {
	BandcampMediaProvider::BandcampMediaProvider(Options options)
	: bandcamp(new Bandcamp(options)) {
		//
	}

	BandcampMediaProvider::~BandcampMediaProvider() {
		delete bandcamp;
	}

	String BandcampMediaProvider::name() const {
		return NAME;
	}

	String BandcampMediaProvider::displayName() const {
		return "Bandcamp";
	}




	Bandcamp* BandcampMediaProvider::api() {
		return bandcamp;
	}

	const Bandcamp* BandcampMediaProvider::api() const {
		return bandcamp;
	}



	#pragma mark URI/URL parsing

	BandcampMediaProvider::URI BandcampMediaProvider::parseURI(String uri) const {
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
		return URI{
			.provider = providerName,
			.url = URL(urlString).toString()
		};
	}

	BandcampMediaProvider::URI BandcampMediaProvider::parseURL(String url) const {
		// validate url
		return URI{
			.provider = name(),
			.url = URL(url).toString()
		};
	}

	String BandcampMediaProvider::createURI(String type, String url) const {
		if(url.empty()) {
			throw std::logic_error("url cannot be empty in bandcamp uri");
		}
		return name()+':'+url;
	}

	String BandcampMediaProvider::idFromUserURL(String url) const {
		if(url.empty()) {
			throw std::invalid_argument("Invalid empty user url");
		}
		auto urlObj = URL(url);
		if(urlObj.host() == "bandcamp.com" || urlObj.host() == "www.bandcamp.com") {
			String id = urlObj.path();
			while(id.startsWith("/")) {
				id = id.substring(1);
			}
			while(id.endsWith("/")) {
				id = id.substring(0, id.length()-1);
			}
			return id;
		} else {
			throw std::invalid_argument("Invalid bandcamp url "+url);
		}
	}



	#pragma mark Login

	Promise<bool> BandcampMediaProvider::login() {
		return bandcamp->login().map([=](bool loggedIn) -> bool {
			if(loggedIn) {
				storeIdentity(std::nullopt);
				setIdentityNeedsRefresh();
				getIdentity();
			}
			return loggedIn;
		});
	}

	void BandcampMediaProvider::logout() {
		bandcamp->logout();
		storeIdentity(std::nullopt);
	}

	bool BandcampMediaProvider::isLoggedIn() const {
		return bandcamp->isLoggedIn();
	}



	#pragma mark Current User

	Promise<ArrayList<String>> BandcampMediaProvider::getCurrentUserURIs() {
		return getIdentity().map([=](Optional<BandcampIdentities> identities) -> ArrayList<String> {
			if(!identities || !identities->fan) {
				return {};
			}
			return { createURI("user", identities->fan->url) };
		});
	}

	String BandcampMediaProvider::getIdentityFilePath() const {
		String sessionPersistKey = bandcamp->getAuth()->getOptions().sessionPersistKey;
		if(sessionPersistKey.empty()) {
			return String();
		}
		return utils::getCacheDirectoryPath()+"/"+name()+"_identity_"+sessionPersistKey+".json";
	}

	Promise<Optional<BandcampIdentities>> BandcampMediaProvider::fetchIdentity() {
		if(!isLoggedIn()) {
			return Promise<Optional<BandcampIdentities>>::resolve(std::nullopt);
		}
		return bandcamp->getMyIdentities().map([=](BandcampIdentities identities) -> Optional<BandcampIdentities> {
			return maybe(identities);
		});
	}



	#pragma mark Search

	Promise<BandcampMediaProvider::SearchResults> BandcampMediaProvider::search(String query, SearchOptions options) {
		return bandcamp->search(query, options).map([=](BandcampSearchResults searchResults) -> SearchResults {
			return SearchResults{
				.prevURL = searchResults.prevURL,
				.nextURL = searchResults.nextURL,
				.items = searchResults.items.where([&](auto& item) {
					return (item.type != BandcampSearchResults::Item::Type::UNKNOWN);
				}).map([&](auto& item) -> $<MediaItem> {
					auto images = (!item.imageURL.empty()) ?
						maybe(ArrayList<MediaItem::Image>{{
							.url=item.imageURL,
							.size=MediaItem::Image::Size::SMALL
						}})
						: std::nullopt;
					switch(item.type) {
						case BandcampSearchResults::Item::Type::TRACK:
							return this->track(Track::Data{{
								.partial = true,
								.type = "track",
								.name = item.name,
								.uri = (item.url.empty() ? String() : createURI("track", item.url)),
								.images = images
								},
								.albumName = item.albumName,
								.albumURI = (item.albumURL.empty() ? String() : createURI("album", item.albumURL)),
								.artists = ArrayList<$<Artist>>{
									this->artist(Artist::Data{{
										.partial=true,
										.type="artist",
										.name=item.artistName,
										.uri=(item.artistURL.empty() ? String() : createURI("artist", item.artistURL)),
										.images=std::nullopt
										},
										.description=std::nullopt
									})
								},
								.tags = item.tags,
								.discNumber = std::nullopt,
								.trackNumber = std::nullopt,
								.duration = std::nullopt,
								.audioSources = std::nullopt,
								.playable = std::nullopt
							});
							
						case BandcampSearchResults::Item::Type::ALBUM:
							return this->album(Album::Data{{{
								.partial = true,
								.type = "album",
								.name = item.name,
								.uri = (item.url.empty() ? String() : createURI("album", item.url)),
								.images = images
								},
								.versionId = String(),
								.itemCount = item.numTracks,
								.items = {}
								},
								.artists = ArrayList<$<Artist>>{
									this->artist(Artist::Data{{
										.partial=true,
										.type="artist",
										.name=item.artistName,
										.uri=(item.artistURL.empty() ? String() : createURI("artist", item.artistURL)),
										.images=std::nullopt
										},
										.description=std::nullopt
									})
								}
							});
							
						case BandcampSearchResults::Item::Type::ARTIST:
							return this->artist(Artist::Data{{
								.partial = true,
								.type = "artist",
								.name = item.name,
								.uri = (item.url.empty() ? String() : createURI("artist", item.url)),
								.images = images
								},
								.description = std::nullopt
							});
							
						case BandcampSearchResults::Item::Type::LABEL:
							return this->artist(Artist::Data{{
								.partial = true,
								.type = "artist",
								.name = item.name,
								.uri = (item.url.empty() ? String() : createURI("artist", item.url)),
								.images = images
								},
								.description = std::nullopt
							});
							
						case BandcampSearchResults::Item::Type::FAN:
							return this->userAccount(UserAccount::Data{
								.partial = true,
								.type = "user",
								.name = (!item.name.empty()) ? item.name : idFromUserURL(item.url),
								.uri = (item.url.empty() ? String() : createURI("user", item.url)),
								.images = images
							});
							
						case BandcampSearchResults::Item::Type::UNKNOWN:
							throw std::logic_error("found unknown bandcamp item");
					}
				})
			};
		});
	}



	#pragma mark Data transforming

	ArrayList<$<Artist>> BandcampMediaProvider::createArtists(String artistURL, String artistName, Optional<BandcampArtist> artist, bool partial) {
		ArrayList<$<Artist>> artists;
		if(artist && !artistURL.empty() && artist->url == artistURL && artist->name != artistName) {
			artistURL = String();
		}
		if(!artist || (artistURL.empty() && !artistName.empty() && artist->name != artistName) || (!artistURL.empty() && artistURL != artist->url)) {
			artists.pushBack(this->artist(Artist::Data{{
				.partial = true,
				.type = "artist",
				.name = artistName,
				.uri = (artistURL.empty() ? String() : createURI("artist", artistURL)),
				.images = std::nullopt
				},
				.description = std::nullopt
			}));
		}
		if(artist) {
			artists.pushBack(this->artist(createArtistData(artist.value(), partial)));
		}
		return artists;
	}

	Track::Data BandcampMediaProvider::createTrackData(BandcampTrack track, bool partial) {
		return Track::Data{{
			.partial = partial,
			.type = "track",
			.name = track.name,
			.uri = (track.url.empty() ? String() : createURI("track", track.url)),
			.images = track.images.map([&](auto& image) -> MediaItem::Image {
				return createImage(image);
			})
			},
			.albumName = track.albumName,
			.albumURI = (track.albumURL.empty() ? String() : createURI("album", track.albumURL)),
			.artists = createArtists(track.artistURL, track.artistName, track.artist, partial),
			.tags = track.tags,
			.discNumber = std::nullopt,
			.trackNumber = track.trackNumber,
			.duration = track.duration,
			.audioSources = (track.audioSources.has_value() ?
				maybe(track.audioSources->map([](auto& audioSource) -> Track::AudioSource {
					String encoding = "mp3";
					double bitrate = 128.0;
					std::cmatch matches;
					std::regex expr("^([\\w\\-]+)\\-([\\d\\.]+)$");
					if(std::regex_match(audioSource.type.c_str(), matches, expr)) {
						encoding = matches[1].str();
						bitrate = String(matches[2].str()).toArithmeticValue<double>();
					}
					return Track::AudioSource{
						.url = audioSource.url,
						.encoding = encoding,
						.bitrate = bitrate
					};
				}))
				: ((track.playable.has_value() && !track.playable.value()) ?
				   maybe(ArrayList<Track::AudioSource>{})
				   : std::nullopt)
				),
			.playable =
				track.playable.hasValue() ? track.playable
				: track.audioSources ? maybe(track.audioSources->size() > 0) : std::nullopt
		};
	}

	Track::Data BandcampMediaProvider::createTrackData(BandcampFan::CollectionTrack track) {
		return Track::Data{{
			.partial=true,
			.type="track",
			.name=track.name,
			.uri=(track.url.empty() ? String() : createURI("track", track.url)),
			.images=(track.images ? maybe(track.images->map([&](auto& image) -> MediaItem::Image {
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
			.playable=std::nullopt
		};
	}

	Artist::Data BandcampMediaProvider::createArtistData(BandcampArtist artist, bool partial) {
		return Artist::Data{{
			.partial=partial,
			.type="artist",
			.name=artist.name,
			.uri=(artist.url.empty() ? String() : createURI("artist", artist.url)),
			.images=artist.images.map([&](auto& image) -> MediaItem::Image {
				return createImage(image);
			})
			},
			.description=artist.description
		};
	}

	Artist::Data BandcampMediaProvider::createArtistData(BandcampFan::CollectionArtist artist) {
		return Artist::Data{{
			.partial=true,
			.type="artist",
			.name=artist.name,
			.uri=(artist.url.empty() ? String() : createURI("artist", artist.url)),
			.images=artist.images ? maybe(artist.images->map([&](auto& image) -> MediaItem::Image {
				return createImage(image);
			})) : std::nullopt
			},
			.description=std::nullopt
		};
	}

	Album::Data BandcampMediaProvider::createAlbumData(BandcampAlbum album, bool partial) {
		auto artists = createArtists(album.artistURL, album.artistName, album.artist, partial);
		auto albumURI = (album.url.empty() ? String() : createURI("album", album.url));
		return Album::Data{{{
			.partial = partial,
			.type = "album",
			.name = album.name,
			.uri = albumURI,
			.images = album.images.map([&](auto& image) -> MediaItem::Image {
				return createImage(std::move(image));
			})
			},
			.versionId = String(),
			.itemCount = album.numTracks,
			.items = (album.tracks ?
				album.tracks->toMap([&](auto& track, size_t index) {
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
					return make_pair(index, AlbumItem::Data{
						.track=this->track(trackData)
					});
				})
				: Map<size_t,AlbumItem::Data>{})
			},
			.artists=artists,
		};
	}

	Album::Data BandcampMediaProvider::createAlbumData(BandcampFan::CollectionAlbum album) {
		return Album::Data{{{
			.partial = true,
			.type = "album",
			.name = album.name,
			.uri = (album.url.empty() ? String() : createURI("album", album.url)),
			.images = album.images ? maybe(album.images->map([&](auto& image) -> MediaItem::Image {
				return createImage(image);
			})) : std::nullopt
			},
			.versionId = String(),
			.itemCount = std::nullopt,
			.items = {}
			},
			.artists = createArtists(album.artistURL, album.artistName, std::nullopt, true)
		};
	}

	MediaItem::Image BandcampMediaProvider::createImage(BandcampImage image) {
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

	UserAccount::Data BandcampMediaProvider::createUserData(BandcampFan fan) {
		return UserAccount::Data{
			.partial = false,
			.type = "user",
			.name = (!fan.name.empty()) ? fan.name : fan.id,
			.uri = (fan.url.empty() ? String() : createURI("fan", fan.url)),
			.images = (fan.images ? maybe(fan.images->map([&](auto& image) -> MediaItem::Image {
				return createImage(std::move(image));
			})) : std::nullopt)
		};
	}

	UserAccount::Data BandcampMediaProvider::createUserData(BandcampFan::CollectionFan fan) {
		return UserAccount::Data{
			.partial = false,
			.type = "user",
			.name = (!fan.name.empty()) ? fan.name : fan.id,
			.uri = (fan.url.empty() ? String() : createURI("fan", fan.url)),
			.images = (fan.images ? maybe(fan.images->map([&](auto& image) -> MediaItem::Image {
				return createImage(std::move(image));
			})) : std::nullopt)
		};
	}



	#pragma mark Media Item Fetching

	Promise<Track::Data> BandcampMediaProvider::getTrackData(String uri) {
		auto uriParts = parseURI(uri);
		return bandcamp->getTrack(uriParts.url).map([=](auto track) -> Track::Data {
			return createTrackData(track, false);
		});
	}

	Promise<Artist::Data> BandcampMediaProvider::getArtistData(String uri) {
		auto uriParts = parseURI(uri);
		return bandcamp->getArtist(uriParts.url).map([=](auto artist) -> Artist::Data {
			return createArtistData(artist, false);
		});
	}

	Promise<Album::Data> BandcampMediaProvider::getAlbumData(String uri) {
		auto uriParts = parseURI(uri);
		return bandcamp->getAlbum(uriParts.url).map([=](auto album) -> Album::Data {
			return createAlbumData(album, false);
		});
	}

	Promise<Playlist::Data> BandcampMediaProvider::getPlaylistData(String uri) {
		return Promise<Playlist::Data>::reject(std::logic_error("Bandcamp does not support playlists"));
	}

	Promise<UserAccount::Data> BandcampMediaProvider::getUserData(String uri) {
		auto uriParts = parseURI(uri);
		return bandcamp->getFan(uriParts.url).map([=](auto fan) -> UserAccount::Data {
			return createUserData(fan);
		});
	}




	Promise<ArrayList<$<Track>>> BandcampMediaProvider::getArtistTopTracks(String artistURI) {
		return Promise<ArrayList<$<Track>>>::reject(std::runtime_error("Bandcamp does not have Top Tracks"));
	}

	Promise<std::tuple<$<Artist>,LinkedList<$<Album>>>> BandcampMediaProvider::getArtistAndAlbums(String artistURI) {
		using ArtistAlbumsTuple = std::tuple<$<Artist>,LinkedList<$<Album>>>;
		auto uriParts = parseURI(artistURI);
		return bandcamp->getArtist(uriParts.url).map([=](auto bcArtist) -> ArtistAlbumsTuple {
			if(!bcArtist.albums) {
				throw std::runtime_error("Failed to parse bandcamp artist's albums");
			}
			auto artist = this->artist(createArtistData(bcArtist, false));
			return ArtistAlbumsTuple{
				artist,
				bcArtist.albums->map([=](auto bcAlbum) -> $<Album> {
					auto data = createAlbumData(bcAlbum, true);
					for(size_t i=0; i<data.artists.size(); i++) {
						auto cmpArtist = data.artists[i];
						if(cmpArtist->uri() == artist->uri()) {
							data.artists[i] = artist;
							break;
						}
					}
					return this->album(data);
				})
			};
		});
	}

	BandcampMediaProvider::ArtistAlbumsGenerator BandcampMediaProvider::getArtistAlbums(String artistURI) {
		using YieldResult = ArtistAlbumsGenerator::YieldResult;
		auto uriParts = parseURI(artistURI);
		return ArtistAlbumsGenerator([=]() {
			return getArtistAndAlbums(uriParts.url).map([=](auto tuple) -> YieldResult {
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

	BandcampMediaProvider::UserPlaylistsGenerator BandcampMediaProvider::getUserPlaylists(String userURI) {
		using YieldResult = typename UserPlaylistsGenerator::YieldResult;
		return UserPlaylistsGenerator([=]() {
			return Promise<YieldResult>::reject(std::runtime_error("Bandcamp does not have user playlists"));
		});
	}




	Album::MutatorDelegate* BandcampMediaProvider::createAlbumMutatorDelegate($<Album> album) {
		return new BandcampAlbumMutatorDelegate(album);
	}

	Playlist::MutatorDelegate* BandcampMediaProvider::createPlaylistMutatorDelegate($<Playlist> playlist) {
		throw std::logic_error("Bandcamp does not support playlists");
	}



	#pragma mark User Library

	auto BandcampMediaProvider_syncLibraryTypes = ArrayList<String>{ "hidden", "collection", "wishlist" };

	BandcampMediaProvider::GenerateLibraryResumeData BandcampMediaProvider::GenerateLibraryResumeData::fromJson(const Json& json) {
		auto mostRecentHiddenSave = json["mostRecentHiddenSave"];
		auto mostRecentCollectionSave = json["mostRecentCollectionSave"];
		auto mostRecentWishlistSave = json["mostRecentWishlistSave"];
		auto syncMostRecentSave = json["syncMostRecentSave"];
		auto resumeData = GenerateLibraryResumeData{
			.fanId = (std::string)json["fanId"].string_value(),
			.mostRecentHiddenSave = mostRecentHiddenSave.is_string() ? Date::maybeFromISOString(mostRecentHiddenSave.string_value()) : std::nullopt,
			.mostRecentCollectionSave = mostRecentCollectionSave.is_string() ? Date::maybeFromISOString(mostRecentCollectionSave.string_value()) : std::nullopt,
			.mostRecentWishlistSave = mostRecentWishlistSave.is_string() ? Date::maybeFromISOString(mostRecentWishlistSave.string_value()) : std::nullopt,
			.syncCurrentType = json["syncCurrentType"].string_value(),
			.syncMostRecentSave = syncMostRecentSave.is_string() ? Date::maybeFromISOString(syncMostRecentSave.string_value()) : std::nullopt,
			.syncLastToken = json["syncLastToken"].string_value(),
			.syncOffset = (size_t)json["syncOffset"].number_value()
		};
		bool syncTypeValid = BandcampMediaProvider_syncLibraryTypes.contains(resumeData.syncCurrentType);
		if(!(resumeData.syncMostRecentSave.has_value() && !resumeData.syncLastToken.empty()
			 && syncTypeValid)) {
			if(!syncTypeValid) {
				resumeData.syncCurrentType = BandcampMediaProvider_syncLibraryTypes[0];
			}
			resumeData.syncMostRecentSave = std::nullopt;
			resumeData.syncLastToken = String();
			resumeData.syncOffset = 0;
		}
		return resumeData;
	}

	Json BandcampMediaProvider::GenerateLibraryResumeData::toJson() const {
		return Json::object{
			{ "fanId", (std::string)fanId },
			{ "mostRecentHiddenSave", mostRecentHiddenSave ? (std::string)mostRecentHiddenSave->toISOString() : Json() },
			{ "mostRecentCollectionSave", mostRecentCollectionSave ? (std::string)mostRecentCollectionSave->toISOString() : Json() },
			{ "mostRecentWishlistSave", mostRecentWishlistSave ? (std::string)mostRecentWishlistSave->toISOString() : Json() },
			{ "syncCurrentType", (std::string)syncCurrentType },
			{ "syncMostRecentSave", syncMostRecentSave ? (std::string)syncMostRecentSave->toISOString() : Json() },
			{ "syncLastToken", (std::string)syncLastToken },
			{ "syncOffset", (double)syncOffset }
		};
	}

	bool BandcampMediaProvider::hasLibrary() const {
		return true;
	}

	BandcampMediaProvider::LibraryItemGenerator BandcampMediaProvider::generateLibrary(GenerateLibraryOptions options) {
		auto resumeData = GenerateLibraryResumeData::fromJson(options.resumeData);
		struct SharedData {
			GenerateLibraryResumeData resumeData;
			
			Optional<BandcampFan> fan;
			double pendingItemsProgress = 0.0;
			double pendingItemProgressDiff = 0.0;
			bool pendingItemsFinishes = false;
			LinkedList<LibraryItem> pendingItems;
			Json pendingItemsResumeData;
			String pendingItemsType = String();
		};
		
		auto sharedData = fgl::new$<SharedData>(SharedData{
			.resumeData = resumeData
		});
		
		using YieldResult = typename LibraryItemGenerator::YieldResult;
		return LibraryItemGenerator(coLambda([=]() -> Promise<YieldResult> {
			auto& resumeData = sharedData->resumeData;
			co_await resumeOnQueue(DispatchQueue::main());
			auto identities = co_await bandcamp->getMyIdentities();
			// verify that bandcamp is logged in
			if(!identities.fan) {
				throw std::runtime_error("No fan identity available");
			}
			// if current user has changed, completely reset the sync data
			if(resumeData.fanId.empty() || resumeData.fanId != identities.fan->id) {
				*sharedData = SharedData();
				resumeData.syncCurrentType = BandcampMediaProvider_syncLibraryTypes[0];
				resumeData.fanId = identities.fan->id;
				sharedData->fan = std::nullopt;
			}
			// fetch fan page data if needed
			if(!sharedData->fan) {
				sharedData->fan = co_await bandcamp->getFan(identities.fan->url);
			}
			// function that dequeues items from the api chunk, fetching data for each if needed
			auto dequeuePendingItems = [=](bool saveAlbumTracks) -> Promise<YieldResult> {
				if(sharedData->pendingItems.size() == 0) {
					co_return YieldResult{
						.value = GenerateLibraryResults{
							.resumeData = sharedData->resumeData.toJson(),
							.items = {},
							.progress = sharedData->pendingItemsProgress
						},
						.done = sharedData->pendingItemsFinishes
					};
				}
				size_t maxItemsCount = 6;
				size_t pendingItemCount = 0;
				auto dequeuedItems = LinkedList<LibraryItem>();
				auto dequeuedIts = LinkedList<LinkedList<LibraryItem>::iterator>();
				double progress = sharedData->pendingItemsProgress;
				for(auto it=sharedData->pendingItems.begin(); it!=sharedData->pendingItems.end(); it++) {
					auto& libraryItem = *it;
					dequeuedIts.pushBack(it);
					dequeuedItems.pushBack(libraryItem);
					pendingItemCount++;
					// fetch missing item data if needed
					if(libraryItem.mediaItem->needsData()) {
						co_await libraryItem.mediaItem->fetchDataIfNeeded();
					}
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
							dequeuedItems.pushBack(LibraryItem{
								.mediaItem = item->track(),
								.addedAt = libraryItem.addedAt
							});
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
				// delete items from pendingItems
				for(auto& it : dequeuedIts) {
					sharedData->pendingItems.erase(it);
				}
				
				// finish
				if(dequeuedAll) {
					bool done = sharedData->pendingItemsFinishes;
					sharedData->pendingItemsType = String();
					sharedData->pendingItemsProgress = 0.0;
					sharedData->pendingItemProgressDiff = 0.0;
					sharedData->pendingItemsResumeData = Json();
					sharedData->pendingItemsFinishes = false;
					sharedData->pendingItems = {};
					co_return YieldResult{
						.value=GenerateLibraryResults{
							.resumeData=sharedData->resumeData.toJson(),
							.items=dequeuedItems,
							.progress=progress
						},
						.done=done
					};
				} else {
					sharedData->pendingItemsProgress = progress;
					co_return YieldResult{
						.value=GenerateLibraryResults{
							.resumeData=sharedData->pendingItemsResumeData,
							.items=dequeuedItems,
							.progress=progress
						},
						.done=false
					};
				}
			};
			// end dequeuePendingItems function
			
			// generate library items
			size_t sectionCount = BandcampMediaProvider_syncLibraryTypes.size();
			// function that generates a page of media items if needed and then dequeues them with dequeuePendingItems
			auto generateSectionMediaItems = [=](const Optional<BandcampFan::Section<BandcampFan::CollectionItemNode>>& collection, Optional<Date>* mostRecentSave, bool saveAlbumTracks, Function<Promise<BandcampFanSectionPage<BandcampFan::CollectionItemNode>>()> fetcher) -> Promise<YieldResult> {
				auto& resumeData = sharedData->resumeData;
				// if we have pending items, dequeue them
				if(sharedData->pendingItems.size() > 0) {
					co_return co_await dequeuePendingItems(saveAlbumTracks);
				}
				
				// function to check if the page catches this section up with the last sync time
				auto checkIndexSectionCaughtUp = [](const LinkedList<LibraryItem>& items, const Optional<Date>& mostRecentSave) -> Optional<size_t> {
					if(!mostRecentSave) {
						return std::nullopt;
					}
					size_t index = 0;
					for(auto& item : items) {
						if(item.addedAt <= mostRecentSave.value()) {
							return index;
						}
						index++;
					}
					return std::nullopt;
				};
				// function to map collection item nodes to library items
				auto mapLibraryItems = [](BandcampMediaProvider* provider, const ArrayList<BandcampFan::CollectionItemNode>& items) -> LinkedList<LibraryItem> {
					LinkedList<LibraryItem> libraryItems;
					for(auto& item : items) {
						if(auto track = item.trackItem()) {
							libraryItems.pushBack(LibraryItem{
								.mediaItem = provider->track(provider->createTrackData(track.value())),
								.addedAt = Date::fromISOString(item.dateAdded)
							});
						} else if(auto album = item.albumItem()) {
							libraryItems.pushBack(LibraryItem{
								.mediaItem = provider->album(provider->createAlbumData(album.value())),
								.addedAt = Date::fromISOString(item.dateAdded)
							});
						}
					}
					return libraryItems;
				};
				
				size_t sectionIndex = BandcampMediaProvider_syncLibraryTypes.indexOf(resumeData.syncCurrentType);
				if(!collection) {
					// collection is empty, so move to next section
					bool done = false;
					*mostRecentSave = std::nullopt;
					sectionIndex += 1;
					resumeData.syncOffset = 0;
					double progress = (double)sectionIndex / (double)sectionCount;
					if(sectionIndex >= sectionCount) {
						resumeData.syncCurrentType = String();
						sectionIndex = 0;
						progress = 1.0;
						done = true;
					} else {
						resumeData.syncCurrentType = BandcampMediaProvider_syncLibraryTypes[sectionIndex];
					}
					resumeData.syncLastToken = String();
					resumeData.syncMostRecentSave = std::nullopt;
					co_return YieldResult{
						.value = GenerateLibraryResults{
							.resumeData = resumeData.toJson(),
							.items = {},
							.progress = progress
						},
						.done = done
					};
				}
				// if we don't have a last token, get the items from the fan page and set the last token
				if(resumeData.syncLastToken.empty()) {
					// save old resume data and progress
					auto oldResumeData = resumeData.toJson();
					double oldSectionProgress = (double)resumeData.syncOffset / (double)collection->itemCount;
					if(oldSectionProgress > 1.0) {
						oldSectionProgress = 1.0;
					}
					double oldProgress = ((double)sectionIndex + oldSectionProgress) / (double)sectionCount;
					auto oldType = resumeData.syncCurrentType;
					// map items
					auto items = mapLibraryItems(this, collection->items);
					// update last token, offset, and initial item save
					if(collection->items.size() > 0) {
						resumeData.syncMostRecentSave = Date::fromISOString(collection->items.front().dateAdded);
					} else {
						resumeData.syncMostRecentSave = std::nullopt;
					}
					resumeData.syncLastToken = collection->lastToken;
					resumeData.syncOffset += collection->items.size();
					double sectionProgress = (double)resumeData.syncOffset / (double)collection->itemCount;
					if(sectionProgress > 1.0) {
						sectionProgress = 1.0;
					}
					double progress = ((double)sectionIndex + sectionProgress) / (double)sectionCount;
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
						*mostRecentSave = resumeData.syncMostRecentSave;
						sectionIndex += 1;
						progress = ((double)sectionIndex / (double)sectionCount);
						if(sectionIndex >= sectionCount) {
							resumeData.syncCurrentType = String();
							sectionIndex = 0;
							progress = 1.0;
							done = true;
						} else {
							resumeData.syncCurrentType = BandcampMediaProvider_syncLibraryTypes[sectionIndex];
						}
						resumeData.syncOffset = 0;
						resumeData.syncLastToken = String();
						resumeData.syncMostRecentSave = std::nullopt;
					}
					// if we don't have any items, return what we have
					if(items.size() == 0) {
						// return fan page items
						co_return YieldResult{
							.value=GenerateLibraryResults{
								.resumeData = resumeData.toJson(),
								.items = items,
								.progress = progress
							},
							.done=done
						};
					}
					// set pending items
					sharedData->pendingItems = items;
					sharedData->pendingItemsType = oldType;
					sharedData->pendingItemsProgress = oldProgress;
					sharedData->pendingItemProgressDiff = (progress - oldProgress) / items.size();
					sharedData->pendingItemsResumeData = oldResumeData;
					sharedData->pendingItemsFinishes = done;
					// dequeue pending items
					co_return co_await dequeuePendingItems(saveAlbumTracks);
				}
				// fetch items
				auto page = co_await fetcher();
				// ensure we have a token for the next page
				if(page.hasMore && page.lastToken.empty()) {
					throw std::runtime_error("missing lastToken for bandcamp library page");
				}
				// save old resume data and progress
				auto oldResumeData = resumeData.toJson();
				double oldSectionProgress = (double)resumeData.syncOffset / (double)collection->itemCount;
				if(oldSectionProgress > 1.0) {
					oldSectionProgress = 1.0;
				}
				double oldProgress = ((double)sectionIndex + oldSectionProgress) / (double)sectionCount;
				auto oldType = resumeData.syncCurrentType;
				// map items and increment offset
				auto items = mapLibraryItems(this, page.items);
				resumeData.syncOffset += items.size();
				// check if sync is finished
				bool done = false;
				double sectionProgress = (double)resumeData.syncOffset / (double)collection->itemCount;
				if(sectionProgress > 1.0) {
					sectionProgress = 1.0;
				}
				double progress = ((double)sectionIndex + sectionProgress) / (double)sectionCount;
				auto caughtUpIndex = checkIndexSectionCaughtUp(items, *mostRecentSave);
				if(caughtUpIndex || !page.hasMore || page.lastToken.empty()) {
					// trim items
					if(caughtUpIndex) {
						items = LinkedList<LibraryItem>(items.begin(), std::next(items.begin(),caughtUpIndex.value()));
					}
					// move to next section
					*mostRecentSave = resumeData.syncMostRecentSave;
					sectionIndex += 1;
					if(sectionIndex >= sectionCount) {
						resumeData.syncCurrentType = String();
						sectionIndex = 0;
						progress = 1.0;
						done = true;
					} else {
						resumeData.syncCurrentType = BandcampMediaProvider_syncLibraryTypes[sectionIndex];
						progress = ((double)sectionIndex / sectionCount);
					}
					resumeData.syncOffset = 0;
					resumeData.syncLastToken = String();
					resumeData.syncMostRecentSave = std::nullopt;
				} else {
					resumeData.syncLastToken = page.lastToken;
				}
				// if we don't have any items, return what we have
				if(items.size() == 0) {
					co_return YieldResult{
						.value=GenerateLibraryResults{
							.resumeData = resumeData.toJson(),
							.items = items,
							.progress = progress
						},
						done = done
					};
				}
				// set pending items
				sharedData->pendingItems = items;
				sharedData->pendingItemsType = oldType;
				sharedData->pendingItemsProgress = oldProgress;
				sharedData->pendingItemProgressDiff = (progress - oldProgress) / items.size();
				sharedData->pendingItemsResumeData = oldResumeData;
				sharedData->pendingItemsFinishes = done;
				// dequeue pending items
				co_return co_await dequeuePendingItems(saveAlbumTracks);
			};
			// end generateSectionMediaItems function
			
			// check section
			auto type = resumeData.syncCurrentType;
			if(sharedData->pendingItems.size() > 0) {
				type = sharedData->pendingItemsType;
			}
			if(type == "hidden") {
				// hidden items
				co_return co_await generateSectionMediaItems(sharedData->fan->hiddenCollection, &resumeData.mostRecentHiddenSave, true, [=]() {
					return bandcamp->getFanHiddenItems(sharedData->fan->url, resumeData.fanId, {
						.olderThanToken = resumeData.syncLastToken,
						.count = sharedData->fan->hiddenCollection->batchSize.valueOr(20)
					});
				});
			}
			else if(type == "collection") {
				// collection items
				co_return co_await generateSectionMediaItems(sharedData->fan->collection, &resumeData.mostRecentCollectionSave, true, [=]() {
					return bandcamp->getFanCollectionItems(sharedData->fan->url, resumeData.fanId, {
						.olderThanToken = resumeData.syncLastToken,
						.count = sharedData->fan->collection->batchSize.valueOr(20)
					});
				});
			}
			else if(type == "wishlist") {
				// wishlist items
				co_return co_await generateSectionMediaItems(sharedData->fan->wishlist, &resumeData.mostRecentWishlistSave, false, [=]() {
					return bandcamp->getFanWishlistItems(sharedData->fan->url, resumeData.fanId, {
						.olderThanToken = resumeData.syncLastToken,
						.count = sharedData->fan->wishlist->batchSize.valueOr(20)
					});
				});
			}
			throw std::runtime_error("invalid bandcamp sync section");
		}));
	}



	bool BandcampMediaProvider::canFollowArtists() const {
		return true;
	}

	Promise<void> BandcampMediaProvider::followArtist(String artistURI) {
		auto uriParts = parseURI(artistURI);
		return bandcamp->followArtist(uriParts.url);
	}

	Promise<void> BandcampMediaProvider::unfollowArtist(String artistURI) {
		auto uriParts = parseURI(artistURI);
		return bandcamp->unfollowArtist(uriParts.url);
	}

	bool BandcampMediaProvider::canFollowUsers() const {
		return true;
	}

	Promise<void> BandcampMediaProvider::followUser(String userURI) {
		auto uriParts = parseURI(userURI);
		return bandcamp->followFan(uriParts.url);
	}

	Promise<void> BandcampMediaProvider::unfollowUser(String userURI) {
		auto uriParts = parseURI(userURI);
		return bandcamp->unfollowFan(uriParts.url);
	}

	bool BandcampMediaProvider::canSaveTracks() const {
		return true;
	}

	Promise<void> BandcampMediaProvider::saveTrack(String trackURI) {
		auto uriParts = parseURI(trackURI);
		return bandcamp->saveItem(uriParts.url);
	}

	Promise<void> BandcampMediaProvider::unsaveTrack(String trackURI) {
		auto uriParts = parseURI(trackURI);
		return bandcamp->unsaveItem(uriParts.url);
	}

	bool BandcampMediaProvider::canSaveAlbums() const {
		return true;
	}

	Promise<void> BandcampMediaProvider::saveAlbum(String albumURI) {
		auto uriParts = parseURI(albumURI);
		return bandcamp->saveItem(uriParts.url);
	}

	Promise<void> BandcampMediaProvider::unsaveAlbum(String albumURI) {
		auto uriParts = parseURI(albumURI);
		return bandcamp->unsaveItem(uriParts.url);
	}

	bool BandcampMediaProvider::canSavePlaylists() const {
		return true;
	}

	Promise<void> BandcampMediaProvider::savePlaylist(String playlistURI) {
		return Promise<void>::reject(std::logic_error("bandcamp does not have playlists"));
	}

	Promise<void> BandcampMediaProvider::unsavePlaylist(String playlistURI) {
		return Promise<void>::reject(std::logic_error("bandcamp does not have playlists"));
	}
}

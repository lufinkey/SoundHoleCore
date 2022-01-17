//
//  LastFMMediaProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/9/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#include "LastFMMediaProvider.hpp"
#include <soundhole/utils/Utils.hpp>

namespace sh {
	LastFMMediaProvider::LastFMMediaProvider(Options options)
	: lastfm(new LastFM(options)) {
		//
	}

	LastFMMediaProvider::~LastFMMediaProvider() {
		//
	}

	String LastFMMediaProvider::name() const {
		return NAME;
	}

	String LastFMMediaProvider::displayName() const {
		return "Last.FM";
	}

	LastFM* LastFMMediaProvider::api() {
		return lastfm;
	}

	const LastFM* LastFMMediaProvider::api() const {
		return lastfm;
	}



	#pragma mark URI parsing

	LastFMMediaProvider::URI LastFMMediaProvider::parseURI(const String& uri) const {
		if(uri.empty()) {
			throw std::invalid_argument("Empty string is not a valid Spotify uri");
		}
		size_t colonIndex1 = uri.indexOf(':');
		if(colonIndex1 == String::npos) {
			throw std::invalid_argument("Invalid "+displayName()+" uri "+uri);
		}
		size_t colonIndex2 = uri.indexOf(':', colonIndex1+1);
		if(colonIndex2 == String::npos) {
			throw std::invalid_argument("Invalid "+displayName()+" uri "+uri);
		}
		auto provider = uri.viewSubstring(0, colonIndex1);
		auto type = uri.viewSubstring(colonIndex1+1, colonIndex2-(colonIndex1+1));
		auto id = uri.viewSubstring(colonIndex2+1);
		if(provider != this->name()) {
			throw std::invalid_argument("Invalid "+displayName()+" uri "+uri);
		}
		return URI{
			.provider = String(provider),
			.type = String(type),
			.id = String(id)
		};
	}


	String LastFMMediaProvider::createURI(const String& type, const String& id) {
		return String::join({ this->name(), type, id }, ":");
	}

	String LastFMMediaProvider::URI::toString() const {
		return String::join({ provider, type, id }, ":");
	}


	LastFMMediaProvider::URI LastFMMediaProvider::parseArtistURL(const String& urlString) const {
		auto url = URL(urlString);
		if(url.host() != "www.last.fm" && url.host() != "last.fm") {
			throw std::invalid_argument("Invalid "+displayName()+" artist URL "+urlString);
		}
		auto parts = url.path().split('/');
		if(!parts.empty() && parts.front().empty()) {
			parts.popFront();
		}
		if(parts.size() < 2 || parts.front() != "music") {
			throw std::invalid_argument("Invalid "+displayName()+" artist URL "+urlString);
		}
		parts.popFront();
		return URI{
			.provider = this->name(),
			.type = "artist",
			.id = parts.front()
		};
	}

	LastFMMediaProvider::URI LastFMMediaProvider::parseTrackURL(const String& urlString) const {
		auto url = URL(urlString);
		if(url.host() != "www.last.fm" && url.host() != "last.fm") {
			throw std::invalid_argument("Invalid "+displayName()+" track URL "+urlString);
		}
		auto parts = url.path().split('/');
		if(!parts.empty() && parts.front().empty()) {
			parts.popFront();
		}
		if(parts.size() < 4 || parts.front() != "music") {
			throw std::invalid_argument("Invalid "+displayName()+" track URL "+urlString);
		}
		parts.popFront();
		if(parts.size() > 3) {
			parts.erase(std::next(parts.begin(), 3), parts.end());
		}
		return URI{
			.provider = this->name(),
			.type = "track",
			.id = String::join(parts, "/")
		};
	}

	LastFMMediaProvider::URI LastFMMediaProvider::parseAlbumURL(const String& urlString) const {
		auto url = URL(urlString);
		if(url.host() != "www.last.fm" && url.host() != "last.fm") {
			throw std::invalid_argument("Invalid "+displayName()+" album URL "+urlString);
		}
		auto parts = url.path().split('/');
		if(!parts.empty() && parts.front().empty()) {
			parts.popFront();
		}
		if(parts.size() < 3 || parts.front() != "music") {
			throw std::invalid_argument("Invalid "+displayName()+" album URL "+urlString);
		}
		parts.popFront();
		if(parts.size() > 2) {
			parts.erase(std::next(parts.begin(), 2), parts.end());
		}
		return URI{
			.provider = this->name(),
			.type = "album",
			.id = String::join(parts, "/")
		};
	}

	LastFMMediaProvider::URI LastFMMediaProvider::parseUserURL(const String& urlString) const {
		auto url = URL(urlString);
		if(url.host() != "www.last.fm" && url.host() != "last.fm") {
			throw std::invalid_argument("Invalid "+displayName()+" user URL "+urlString);
		}
		auto parts = url.path().split('/');
		if(!parts.empty() && parts.front().empty()) {
			parts.popFront();
		}
		if(parts.size() < 2 || parts.front() != "music") {
			throw std::invalid_argument("Invalid "+displayName()+" artist URL "+urlString);
		}
		parts.popFront();
		return URI{
			.provider = this->name(),
			.type = "user",
			.id = parts.front()
		};
	}



	#pragma mark Login

	Promise<bool> LastFMMediaProvider::login() {
		return lastfm->login().map([=](bool loggedIn) -> bool {
			if(loggedIn) {
				storeIdentity(std::nullopt);
				setIdentityNeedsRefresh();
				getIdentity();
			}
			return loggedIn;
		});
	}

	void LastFMMediaProvider::logout() {
		lastfm->logout();
		storeIdentity(std::nullopt);
	}

	bool LastFMMediaProvider::isLoggedIn() const {
		return lastfm->isLoggedIn();
	}



	#pragma mark Current User

	Promise<ArrayList<String>> LastFMMediaProvider::getCurrentUserURIs() {
		return getIdentity().map([=](Optional<LastFMUserInfo> user) -> ArrayList<String> {
			if(!user) {
				return {};
			}
			return { parseUserURL(user->url).toString() };
		});
	}

	String LastFMMediaProvider::getIdentityFilePath() const {
		String sessionPersistKey = lastfm->getAuth()->options().sessionPersistKey;
		if(sessionPersistKey.empty()) {
			return String();
		}
		return utils::getCacheDirectoryPath()+"/"+name()+"_identity_"+sessionPersistKey+".json";
	}

	Promise<Optional<LastFMUserInfo>> LastFMMediaProvider::fetchIdentity() {
		if(!isLoggedIn()) {
			return Promise<Optional<LastFMUserInfo>>::resolve(std::nullopt);
		}
		return lastfm->getUserInfo().map([=](LastFMUserInfo user) -> Optional<LastFMUserInfo> {
			return maybe(user);
		});
	}



	#pragma mark Data transforming

	Track::Data LastFMMediaProvider::createTrackData(LastFMTrackInfo track) {
		return Track::Data{{
			.partial = false,
			.type = "track",
			.name = track.name,
			.uri = parseTrackURL(track.url).toString(),
			.images = (track.album ?
				maybe(track.album->image.map([&](auto& image) -> MediaItem::Image {
					return createImage(std::move(image));
				}))
				: std::nullopt)
			},
			.albumName = (track.album ? track.album->title : String()),
			.albumURI = (track.album ? parseAlbumURL(track.album->url).toString() : String()),
			.artists = {
				this->artist(createArtistData(track.artist))
			},
			.tags = track.topTags ? maybe(track.topTags->map([](auto& tag) {
				return tag.name;
			})) : std::nullopt,
			.discNumber = std::nullopt,
			.trackNumber = std::nullopt,
			.duration = track.duration.hasValue() ? maybe(track.duration.value() / 1000.0) : std::nullopt,
			.audioSources = std::nullopt,
			.playable = false
		};
	}

	Track::Data LastFMMediaProvider::createTrackData(LastFMTrackSearchResults::Item track) {
		return Track::Data{{
			.partial = true,
			.type = "track",
			.name = track.name,
			.uri = parseTrackURL(track.url).toString(),
			.images = track.image.map([&](auto& image) -> MediaItem::Image {
				return createImage(std::move(image));
			})
			},
			.albumName = String(),
			.albumURI = String(),
			.artists = {
				this->artist(createArtistData(LastFMPartialArtistInfo::fromJson((std::string)track.artist)))
			},
			.tags = std::nullopt,
			.discNumber = std::nullopt,
			.trackNumber = std::nullopt,
			.duration = std::nullopt,
			.audioSources = std::nullopt,
			.playable = false
		};
	}

	Artist::Data LastFMMediaProvider::createArtistData(LastFMArtistInfo artist) {
		return Artist::Data{{
			.partial = false,
			.type = "artist",
			.name = artist.name,
			.uri = parseArtistURL(artist.url).toString(),
			.images = artist.image.map([&](LastFMImage& image) -> MediaItem::Image {
				return createImage(std::move(image));
			})
			},
			.description = artist.bio.content
		};
	}

	Artist::Data LastFMMediaProvider::createArtistData(LastFMPartialArtistInfo artist) {
		return Artist::Data{{
			.partial = true,
			.type = "artist",
			.name = artist.name,
			.uri = parseArtistURL(artist.url).toString(),
			.images = artist.image ? maybe(artist.image->map([&](LastFMImage& image) -> MediaItem::Image {
				return createImage(std::move(image));
			})) : std::nullopt
			},
			.description = std::nullopt
		};
	}

	Artist::Data LastFMMediaProvider::createArtistData(LastFMArtistSearchResults::Item artist) {
		return Artist::Data{{
			.partial = true,
			.type = "artist",
			.name = artist.name,
			.uri = parseArtistURL(artist.url).toString(),
			.images = artist.image.map([&](LastFMImage& image) -> MediaItem::Image {
				return createImage(std::move(image));
			})
			},
			.description = std::nullopt
		};
	}

	Album::Data LastFMMediaProvider::createAlbumData(LastFMAlbumInfo album) {
		auto albumURI = parseAlbumURL(album.url).toString();
		auto albumArtist = ([&]() {
			auto albumArtistURI = parseArtistURL(album.url);
			auto matchingTrack = album.tracks.firstWhere([&](auto& track) {
				return (parseArtistURL(track.artist.url).id == albumArtistURI.id);
			});
			if(matchingTrack) {
				return this->artist(createArtistData(matchingTrack.value().artist));
			}
			auto artistData = createArtistData(LastFMPartialArtistInfo::fromJson((std::string)album.artist));
			artistData.uri = albumArtistURI.toString();
			return this->artist(artistData);
		})();
		auto albumImages = album.image.map([&](LastFMImage& image) -> MediaItem::Image {
			return createImage(std::move(image));
		});
		return Album::Data{{{
			.partial = false,
			.type = "album",
			.name = album.name,
			.uri = albumURI,
			.images = albumImages
			},
			.versionId = String(),
			.itemCount = album.tracks.size(),
			.items = album.tracks.toMap([&](auto& track, size_t index) {
				return std::make_pair(index, AlbumItem::Data{
					.track = this->track({{
						.partial = true,
						.type = "track",
						.name = track.name,
						.uri = parseTrackURL(track.url).toString(),
						.images = albumImages
						},
						.albumName = album.name,
						.albumURI = albumURI,
						.artists = {
							([&]() {
								auto artistData = createArtistData(track.artist);
								if(artistData.uri == albumArtist->uri() && artistData.name == albumArtist->name()) {
									return albumArtist;
								}
								return this->artist(artistData);
							})()
						},
						.tags = std::nullopt,
						.discNumber = std::nullopt,
						.trackNumber = std::nullopt,
						.duration = track.duration,
						.audioSources = std::nullopt,
						.playable = false
					})
				});
			})
			},
			.artists = {
				albumArtist
			}
		};
	}

	Album::Data LastFMMediaProvider::createAlbumData(LastFMArtistTopAlbum album) {
		return Album::Data{{{
			.partial = true,
			.type = "album",
			.name = album.name,
			.uri = parseAlbumURL(album.url).toString(),
			.images = album.image.map([&](LastFMImage& image) -> MediaItem::Image {
				return createImage(std::move(image));
			})
			},
			.versionId = String(),
			.itemCount = std::nullopt,
			.items = {}
			},
			.artists = {
				this->artist(createArtistData(album.artist))
			}
		};
	}

	UserAccount::Data LastFMMediaProvider::createUserAccountData(LastFMUserInfo user) {
		return UserAccount::Data{
			.partial = false,
			.type = "user",
			.name = !user.realname.empty() ? user.realname : user.name,
			.uri = parseUserURL(user.url).toString(),
			.images = user.image.map([&](LastFMImage& image) -> MediaItem::Image {
				return createImage(std::move(image));
			})
		};
	}

	MediaItem::Image LastFMMediaProvider::createImage(LastFMImage image) {
		return MediaItem::Image{
			.url = image.url,
			.size = ([&]() {
				if(image.size.empty()) {
					return MediaItem::Image::Size::LARGE;
				}
				else if(image.size == "small") {
					return MediaItem::Image::Size::SMALL;
				}
				else if(image.size == "medium") {
					return MediaItem::Image::Size::MEDIUM;
				}
				else if(image.size == "large" || image.size == "extralarge" || image.size == "mega") {
					return MediaItem::Image::Size::LARGE;
				}
				return MediaItem::Image::Size::MEDIUM;
			})(),
			.dimensions = std::nullopt
		};
	}



	#pragma mark Media Item Fetching

	Promise<Track::Data> LastFMMediaProvider::getTrackData(String uri) {
		auto uriParts = parseURI(uri);
		auto idParts = uriParts.id.split('/');
		auto session = lastfm->getAuth()->session();
		return lastfm->getTrackInfo({
			.track = URL::decodeQueryValue(idParts.back()),
			.artist = URL::decodeQueryValue(idParts.front()),
			.username = session ? session->name : String(),
			.autocorrect = true
		}).map(nullptr, [=](LastFMTrackInfo track) -> Track::Data {
			return createTrackData(track);
		});
	}

	Promise<Artist::Data> LastFMMediaProvider::getArtistData(String uri) {
		auto uriParts = parseURI(uri);
		auto session = lastfm->getAuth()->session();
		return lastfm->getArtistInfo({
			.artist = URL::decodeQueryValue(uriParts.id),
			.username = session ? session->name : String(),
			.autocorrect = true
		}).map(nullptr, [=](LastFMArtistInfo artist) -> Artist::Data {
			return createArtistData(artist);
		});
	}

	Promise<Album::Data> LastFMMediaProvider::getAlbumData(String uri) {
		auto uriParts = parseURI(uri);
		auto idParts = uriParts.id.split('/');
		return lastfm->getAlbumInfo({
			.album = URL::decodeQueryValue(idParts.back()),
			.artist = URL::decodeQueryValue(idParts.front())
		}).map(nullptr, [=](LastFMAlbumInfo album) -> Album::Data {
			return createAlbumData(album);
		});
	}

	Promise<Playlist::Data> LastFMMediaProvider::getPlaylistData(String uri) {
		return rejectWith(std::runtime_error(displayName()+" has deprecated their playlists API"));
	}

	Promise<UserAccount::Data> LastFMMediaProvider::getUserData(String uri) {
		auto uriParts = parseURI(uri);
		return lastfm->getUserInfo(URL::decodeQueryValue(uriParts.id))
		.map(nullptr, [=](LastFMUserInfo user) -> UserAccount::Data {
			return createUserAccountData(user);
		});
	}



	Promise<ArrayList<$<Track>>> LastFMMediaProvider::getArtistTopTracks(String artistURI) {
		auto uriParts = parseURI(artistURI);
		return lastfm->getArtistTopTracks({
			.artist = URL::decodeQueryValue(uriParts.id),
			.autocorrect = true,
			.limit = 10
		}).map(nullptr, [=](auto page) -> ArrayList<$<Track>> {
			return page.items.map([&](auto& track) -> $<Track> {
				return this->track(createTrackData(std::move(track)));
			});
		});
	}

	LastFMMediaProvider::ArtistAlbumsGenerator LastFMMediaProvider::getArtistAlbums(String artistURI) {
		auto uriParts = parseURI(artistURI);
		auto artistName = URL::decodeQueryValue(uriParts.id);
		auto pageNum = fgl::new$<size_t>(1);
		using YieldResult = typename ArtistAlbumsGenerator::YieldResult;
		return ArtistAlbumsGenerator([=]() {
			return lastfm->getArtistTopAlbums({
				.artist = artistName,
				.page = *pageNum,
				.limit = 20
			}).map(nullptr, [=](auto page) -> YieldResult {
				auto loadBatch = LoadBatch<$<Album>>{
					.items=page.items.template map([=](auto& album) -> $<Album> {
						return this->album(createAlbumData(album));
					}),
					.total=page.attrs.total
				};
				bool done = (*pageNum >= page.attrs.totalPages);
				*pageNum += 1;
				return YieldResult{
					.value=loadBatch,
					.done=done
				};
			});
		});
	}

	LastFMMediaProvider::UserPlaylistsGenerator LastFMMediaProvider::getUserPlaylists(String userURI) {
		throw std::runtime_error(displayName()+" has deprecated their playlists API");
	}



	Album::MutatorDelegate* LastFMMediaProvider::createAlbumMutatorDelegate($<Album> album) {
		return nullptr;
	}

	Playlist::MutatorDelegate* LastFMMediaProvider::createPlaylistMutatorDelegate($<Playlist> playlist) {
		throw std::runtime_error(displayName()+" has deprecated their playlists API");
	}



	#pragma mark User Library

	bool LastFMMediaProvider::hasLibrary() const {
		return false;
	}

	LastFMMediaProvider::LibraryItemGenerator LastFMMediaProvider::generateLibrary(GenerateLibraryOptions options) {
		throw std::runtime_error("LastFMMediaProvider cannot generate a user library");
	}

	bool LastFMMediaProvider::canFollowArtists() const {
		// return true to ensure code doesn't attempt to save local media library artists elsewhere successfully
		return true;
	}

	Promise<void> LastFMMediaProvider::followArtist(String artistURI) {
		return Promise<void>::reject(std::runtime_error("LastFMMediaProvider cannot follow artists"));
	}

	Promise<void> LastFMMediaProvider::unfollowArtist(String artistURI) {
		return Promise<void>::reject(std::runtime_error("LastFMMediaProvider cannot unfollow artists"));
	}

	bool LastFMMediaProvider::canFollowUsers() const {
		// return true to ensure code doesn't attempt to save local media library users elsewhere successfully
		return true;
	}

	Promise<void> LastFMMediaProvider::followUser(String userURI) {
		return Promise<void>::reject(std::runtime_error("LastFMMediaProvider cannot follow users"));
	}

	Promise<void> LastFMMediaProvider::unfollowUser(String userURI) {
		return Promise<void>::reject(std::runtime_error("LastFMMediaProvider cannot unfollow users"));
	}

	bool LastFMMediaProvider::canSaveTracks() const {
		// return true to ensure code doesn't attempt to save local media library playlists elsewhere successfully
		return true;
	}

	Promise<void> LastFMMediaProvider::saveTrack(String trackURI) {
		return Promise<void>::reject(std::runtime_error("LastFMMediaProvider cannot save tracks"));
	}

	Promise<void> LastFMMediaProvider::unsaveTrack(String trackURI) {
		return Promise<void>::reject(std::runtime_error("LastFMMediaProvider cannot unsave tracks"));
	}

	bool LastFMMediaProvider::canSaveAlbums() const {
		// return true to ensure code doesn't attempt to save local media library playlists elsewhere successfully
		return true;
	}

	Promise<void> LastFMMediaProvider::saveAlbum(String albumURI) {
		return Promise<void>::reject(std::runtime_error("LastFMMediaProvider cannot save tracks"));
	}

	Promise<void> LastFMMediaProvider::unsaveAlbum(String albumURI) {
		return Promise<void>::reject(std::runtime_error("LastFMMediaProvider cannot unsave tracks"));
	}

	bool LastFMMediaProvider::canSavePlaylists() const {
		// return true to ensure code doesn't attempt to save local media library playlists elsewhere successfully
		return true;
	}

	Promise<void> LastFMMediaProvider::savePlaylist(String playlistURI) {
		return Promise<void>::reject(std::runtime_error("LastFMMediaProvider cannot save playlists"));
	}

	Promise<void> LastFMMediaProvider::unsavePlaylist(String playlistURI) {
		return Promise<void>::reject(std::runtime_error("LastFMMediaProvider cannot unsave playlists"));
	}



	#pragma mark Scrobbler

	size_t LastFMMediaProvider::maxScrobblesPerRequest() const {
		return 50;
	}

	Promise<ArrayList<Scrobble::Response>> LastFMMediaProvider::scrobble(ArrayList<$<Scrobble>> scrobbles) {
		return lastfm->scrobble({
			.items = scrobbles.map([](auto& scrobble) {
				return LastFMScrobbleRequest::Item{
					.track = scrobble->trackName(),
					.artist = scrobble->artistName(),
					.album = scrobble->albumName(),
					.albumArtist = scrobble->albumArtistName(),
					.timestamp = scrobble->startTime(),
					.chosenByUser = scrobble->chosenByUser(),
					.duration = scrobble->duration(),
					.trackNumber = scrobble->trackNumber(),
					.mbid = scrobble->musicBrainzID()
				};
			})
		}).map(nullptr, [=](auto results) {
			return results.scrobbles.map([](auto& result) {
				return Scrobble::Response{
					.track = {
						.text = result.track.text,
						.corrected = result.track.corrected
					},
					.artist = {
						.text = result.artist.text,
						.corrected = result.artist.corrected
					},
					.album = {
						.text = result.album.text,
						.corrected = result.album.corrected
					},
					.albumArtist = {
						.text = result.albumArtist.text,
						.corrected = result.albumArtist.corrected
					},
					.timestamp = result.timestamp,
					.ignored = (result.ignoredMessage.code != "0" && !result.ignoredMessage.code.trim().empty()) ? maybe(Scrobble::IgnoredReason{
						.code = ignoredScrobbleCodeFromString(result.ignoredMessage.code),
						.message = result.ignoredMessage.text
					}) : std::nullopt
				};
			});
		});
	}

	Scrobble::IgnoredReason::Code LastFMMediaProvider::ignoredScrobbleCodeFromString(const String& str) {
		using Code = Scrobble::IgnoredReason::Code;
		if(str == "1") {
			return Code::IGNORED_ARTIST;
		} else if(str == "2") {
			return Code::IGNORED_TRACK;
		} else if(str == "3") {
			return Code::TIMESTAMP_TOO_OLD;
		} else if(str == "4") {
			return Code::TIMESTAMP_TOO_NEW;
		} else if(str == "5") {
			return Code::DAILY_SCROBBLE_LIMIT_EXCEEDED;
		}
		return Code::UNKNOWN_ERROR;
	}


	Promise<void> LastFMMediaProvider::loveTrack($<Track> track) {
		auto& artists = track->artists();
		return lastfm->loveTrack(track->name(), artists.empty() ? String() : artists.front()->name());
	}

	Promise<void> LastFMMediaProvider::unloveTrack($<Track> track) {
		auto& artists = track->artists();
		return lastfm->unloveTrack(track->name(), artists.empty() ? String() : artists.front()->name());
	}
}

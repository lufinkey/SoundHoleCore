//
//  MediaProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include <napi.h>
#include "MediaProvider.hpp"

namespace sh {
	MediaProvider::LibraryItem MediaProvider::LibraryItem::fromJson(Json json, MediaProviderStash* stash) {
		FGL_ASSERT(stash != nullptr, "stash cannot be null");
		auto mediaItem = json["mediaItem"];
		if(!mediaItem.is_object()) {
			throw std::invalid_argument("mediaItem must be an object");
		}
		return LibraryItem{
			.mediaItem=stash->parseMediaItem(mediaItem),
			.addedAt=json["addedAt"].string_value()
		};
	}



	Promise<ArrayList<Track::Data>> MediaProvider::getTracksData(ArrayList<String> uris) {
		// TODO handle 429 responses and try again
		return Promise<Track::Data>::all(uris.map([&](auto& uri) {
			return this->getTrackData(uri);
		}));
	}
	Promise<ArrayList<Artist::Data>> MediaProvider::getArtistsData(ArrayList<String> uris) {
		// TODO handle 429 responses and try again
		return Promise<Artist::Data>::all(uris.map([&](auto& uri) {
			return this->getArtistData(uri);
		}));
	}
	Promise<ArrayList<Album::Data>> MediaProvider::getAlbumsData(ArrayList<String> uris) {
		// TODO handle 429 responses and try again
		return Promise<Album::Data>::all(uris.map([&](auto& uri) {
			return this->getAlbumData(uri);
		}));
	}
	Promise<ArrayList<Playlist::Data>> MediaProvider::getPlaylistsData(ArrayList<String> uris) {
		// TODO handle 429 responses and try again
		return Promise<Playlist::Data>::all(uris.map([&](auto& uri) {
			return this->getPlaylistData(uri);
		}));
	}
	Promise<ArrayList<UserAccount::Data>> MediaProvider::getUsersData(ArrayList<String> uris) {
		// TODO handle 429 responses and try again
		return Promise<UserAccount::Data>::all(uris.map([&](auto& uri) {
			return this->getUserData(uri);
		}));
	}



	Promise<$<Track>> MediaProvider::getTrack(String uri) {
		return getTrackData(uri).map(nullptr, [=](auto data) -> $<Track> {
			return this->track(data);
		});
	}
	Promise<$<Artist>> MediaProvider::getArtist(String uri) {
		return getArtistData(uri).map(nullptr, [=](auto data) -> $<Artist> {
			return this->artist(data);
		});
	}
	Promise<$<Album>> MediaProvider::getAlbum(String uri) {
		return getAlbumData(uri).map(nullptr, [=](auto data) -> $<Album> {
			return this->album(data);
		});
	}
	Promise<$<Playlist>> MediaProvider::getPlaylist(String uri) {
		return getPlaylistData(uri).map(nullptr, [=](auto data) -> $<Playlist> {
			return this->playlist(data);
		});
	}
	Promise<$<UserAccount>> MediaProvider::getUser(String uri) {
		return getUserData(uri).map(nullptr, [=](auto data) -> $<UserAccount> {
			return this->userAccount(data);
		});
	}



	Promise<ArrayList<$<Track>>> MediaProvider::getTracks(ArrayList<String> uris) {
		return getTracksData(uris).map(nullptr, [=](auto datas) {
			return datas.map([=](auto& data) {
				return this->track(data);
			});
		});
	}
	Promise<ArrayList<$<Artist>>> MediaProvider::getArtists(ArrayList<String> uris) {
		return getArtistsData(uris).map(nullptr, [=](auto datas) {
			return datas.map([=](auto& data) {
				return this->artist(data);
			});
		});
	}
	Promise<ArrayList<$<Album>>> MediaProvider::getAlbums(ArrayList<String> uris) {
		return getAlbumsData(uris).map(nullptr, [=](auto datas) {
			return datas.map([=](auto& data) {
				return this->album(data);
			});
		});
	}
	Promise<ArrayList<$<Playlist>>> MediaProvider::getPlaylists(ArrayList<String> uris) {
		return getPlaylistsData(uris).map(nullptr, [=](auto datas) {
			return datas.map([=](auto& data) {
				return this->playlist(data);
			});
		});
	}
	Promise<ArrayList<$<UserAccount>>> MediaProvider::getUsers(ArrayList<String> uris) {
		return getUsersData(uris).map(nullptr, [=](auto datas) {
			return datas.map([=](auto& data) {
				return this->userAccount(data);
			});
		});
	}



	$<Track> MediaProvider::track(const Track::Data& data) {
		return Track::new$(this, data);
	}

	$<Artist> MediaProvider::artist(const Artist::Data& data) {
		return Artist::new$(this, data);
	}

	$<Album> MediaProvider::album(const Album::Data& data) {
		return Album::new$(this, data);
	}

	$<Playlist> MediaProvider::playlist(const Playlist::Data& data) {
		return Playlist::new$(this, data);
	}

	$<UserAccount> MediaProvider::userAccount(const UserAccount::Data& data) {
		return UserAccount::new$(this, data);
	}



	bool MediaProvider::handlesUsersAsArtists() const {
		return false;
	}

	bool MediaProvider::canCreatePlaylists() const {
		return false;
	}

	ArrayList<Playlist::Privacy> MediaProvider::supportedPlaylistPrivacies() const {
		return {};
	}

	Promise<$<Playlist>> MediaProvider::createPlaylist(String name, CreatePlaylistOptions options) {
		return Promise<$<Playlist>>::reject(std::logic_error("createPlaylist is not implemented"));
	}

	Promise<bool> MediaProvider::isPlaylistEditable($<Playlist> playlist) {
		return Promise<bool>::resolve(false);
	}

	Promise<void> MediaProvider::deletePlaylist(String playlistURI) {
		return Promise<void>::reject(std::logic_error("deletePlaylist is not implemented"));
	}



	Json MediaProvider::CreatePlaylistOptions::toJson() const {
		return Json::object{
			{ "privacy", (std::string)Playlist::Privacy_toString(privacy) }
		};
	}

	#ifdef NODE_API_MODULE
	Napi::Object MediaProvider::CreatePlaylistOptions::toNapiObject(napi_env env) const {
		auto obj = Napi::Object::New(env);
		obj.Set("privacy", Napi::String::New(env, Playlist::Privacy_toString(privacy)));
		return obj;
	}
	#endif
}

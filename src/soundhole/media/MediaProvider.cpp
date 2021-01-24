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
		auto libraryProvider = json["libraryProvider"];
		if(!libraryProvider.is_string()) {
			throw std::invalid_argument("libraryProvider must be a string");
		}
		auto mediaItem = json["mediaItem"];
		if(!mediaItem.is_object()) {
			throw std::invalid_argument("mediaItem must be an object");
		}
		return LibraryItem{
			.libraryProvider=stash->getMediaProvider(libraryProvider.string_value()),
			.mediaItem=stash->parseMediaItem(mediaItem),
			.addedAt=json["addedAt"].string_value()
		};
	}

	Promise<$<Track>> MediaProvider::getTrack(String uri) {
		return getTrackData(uri).map<$<Track>>(nullptr, [=](auto data) {
			return track(data);
		});
	}

	Promise<$<Artist>> MediaProvider::getArtist(String uri) {
		return getArtistData(uri).map<$<Artist>>(nullptr, [=](auto data) {
			return artist(data);
		});
	}

	Promise<$<Album>> MediaProvider::getAlbum(String uri) {
		return getAlbumData(uri).map<$<Album>>(nullptr, [=](auto data) {
			return album(data);
		});
	}

	Promise<$<Playlist>> MediaProvider::getPlaylist(String uri) {
		return getPlaylistData(uri).map<$<Playlist>>(nullptr, [=](auto data) {
			return playlist(data);
		});
	}

	Promise<$<UserAccount>> MediaProvider::getUser(String uri) {
		return getUserData(uri).map<$<UserAccount>>(nullptr, [=](auto data) {
			return userAccount(data);
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

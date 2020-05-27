//
//  MediaLibraryProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 5/26/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "MediaLibraryProvider.hpp"

namespace sh {
	MediaLibraryProvider::MediaLibraryProvider(ArrayList<MediaProvider*> mediaProviders)
	: mediaProviders(mediaProviders) {
		//
	}

	String MediaLibraryProvider::name() const {
		return "medialibrary";
	}

	String MediaLibraryProvider::displayName() const {
		return "MediaLibrary";
	}
	
	Promise<bool> MediaLibraryProvider::login() {
		return Promise<bool>::reject(std::runtime_error("MediaLibraryProvider has no login"));
	}

	void MediaLibraryProvider::logout() {
		//
	}

	bool MediaLibraryProvider::isLoggedIn() const {
		return false;
	}

	Promise<Track::Data> MediaLibraryProvider::getTrackData(String uri) {
		return Promise<Track::Data>::reject(std::runtime_error("MediaLibraryProvider cannot fetch tracks"));
	}

	Promise<Artist::Data> MediaLibraryProvider::getArtistData(String uri) {
		return Promise<Artist::Data>::reject(std::runtime_error("MediaLibraryProvider cannot fetch artists"));
	}

	Promise<Album::Data> MediaLibraryProvider::getAlbumData(String uri) {
		return Promise<Album::Data>::reject(std::runtime_error("MediaLibraryProvider cannot fetch albums"));
	}

	Promise<Playlist::Data> MediaLibraryProvider::getPlaylistData(String uri) {
		return Promise<Playlist::Data>::reject(std::runtime_error("MediaLibraryProvider cannot fetch playlists"));
	}

	Promise<UserAccount::Data> MediaLibraryProvider::getUserData(String uri) {
		return Promise<UserAccount::Data>::reject(std::runtime_error("MediaLibraryProvider cannot fetch users"));
	}

	Promise<ArrayList<$<Track>>> MediaLibraryProvider::getArtistTopTracks(String artistURI) {
		return Promise<ArrayList<$<Track>>>::reject(std::runtime_error("MediaLibraryProvider cannot fetch artist top tracks"));
	}

	MediaLibraryProvider::ArtistAlbumsGenerator MediaLibraryProvider::getArtistAlbums(String artistURI) {
		throw std::runtime_error("MediaLibraryProvider cannot fetch artist albums");
	}
	
	MediaLibraryProvider::UserPlaylistsGenerator MediaLibraryProvider::getUserPlaylists(String userURI) {
		throw std::runtime_error("MediaLibraryProvider cannot fetch user playlists");
	}
	
	Album::MutatorDelegate* MediaLibraryProvider::createAlbumMutatorDelegate($<Album> album) {
		return nullptr;
	}

	Playlist::MutatorDelegate* MediaLibraryProvider::createPlaylistMutatorDelegate($<Playlist> playlist) {
		return nullptr;
	}
	
	MediaLibraryProvider::LibraryItemGenerator MediaLibraryProvider::generateLibrary(GenerateLibraryOptions options) {
		throw std::runtime_error("MediaLibraryProvider cannot generate library");
	}
	
	MediaPlaybackProvider* MediaLibraryProvider::player() {
		return nullptr;
	}

	const MediaPlaybackProvider* MediaLibraryProvider::player() const {
		return nullptr;
	}

	$<MediaItem> MediaLibraryProvider::createMediaItem(Json json) {
		if(json.is_null()) {
			return nullptr;
		}
		auto type = json["type"].string_value();
		if(type.empty()) {
			throw std::invalid_argument("No type stored for media item json");
		}
		if(type == "track") {
			return Track::fromJson(json, this);
		}
		else if(type == "artist" || type == "label") {
			return Artist::fromJson(json, this);
		}
		else if(type == "album") {
			return Album::fromJson(json, this);
		}
		else if(type == "playlist") {
			return Playlist::fromJson(json, this);
		}
		throw std::invalid_argument("invalid media item type "+type);
	}

	ArrayList<MediaProvider*> MediaLibraryProvider::getMediaProviders() {
		return mediaProviders;
	}

	void MediaLibraryProvider::addMediaProvider(MediaProvider* mediaProvider) {
		mediaProviders.pushBack(mediaProvider);
	}

	void MediaLibraryProvider::removeMediaProvider(MediaProvider* mediaProvider) {
		mediaProviders.removeLastEqual(mediaProvider);
	}

	MediaProvider* MediaLibraryProvider::getMediaProvider(const String& name) {
		for(auto provider : mediaProviders) {
			if(provider->name() == name) {
				return provider;
			}
		}
		if(name == this->name()) {
			return this;
		}
		return nullptr;
	}
}

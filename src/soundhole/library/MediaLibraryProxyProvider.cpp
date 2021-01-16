//
//  MediaLibraryProxyProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 5/26/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "MediaLibraryProxyProvider.hpp"

namespace sh {
	MediaLibraryProxyProvider::MediaLibraryProxyProvider(ArrayList<MediaProvider*> mediaProviders)
	: mediaProviders(mediaProviders) {
		//
	}

	MediaLibraryProxyProvider::~MediaLibraryProxyProvider() {
		for(auto provider : mediaProviders) {
			delete provider;
		}
	}

	String MediaLibraryProxyProvider::name() const {
		return "localmedialibrary";
	}

	String MediaLibraryProxyProvider::displayName() const {
		return "Media Library";
	}
	
	Promise<bool> MediaLibraryProxyProvider::login() {
		return Promise<bool>::reject(std::runtime_error("MediaLibraryProxyProvider has no login"));
	}

	void MediaLibraryProxyProvider::logout() {
		//
	}

	bool MediaLibraryProxyProvider::isLoggedIn() const {
		return false;
	}

	Promise<ArrayList<String>> MediaLibraryProxyProvider::getCurrentUserURIs() {
		return Promise<ArrayList<String>>::resolve({});
	}

	Promise<Track::Data> MediaLibraryProxyProvider::getTrackData(String uri) {
		return Promise<Track::Data>::reject(std::runtime_error("MediaLibraryProxyProvider cannot fetch tracks"));
	}

	Promise<Artist::Data> MediaLibraryProxyProvider::getArtistData(String uri) {
		return Promise<Artist::Data>::reject(std::runtime_error("MediaLibraryProxyProvider cannot fetch artists"));
	}

	Promise<Album::Data> MediaLibraryProxyProvider::getAlbumData(String uri) {
		return Promise<Album::Data>::reject(std::runtime_error("MediaLibraryProxyProvider cannot fetch albums"));
	}

	Promise<Playlist::Data> MediaLibraryProxyProvider::getPlaylistData(String uri) {
		return Promise<Playlist::Data>::reject(std::runtime_error("MediaLibraryProxyProvider cannot fetch playlists"));
	}

	Promise<UserAccount::Data> MediaLibraryProxyProvider::getUserData(String uri) {
		return Promise<UserAccount::Data>::reject(std::runtime_error("MediaLibraryProxyProvider cannot fetch users"));
	}

	Promise<ArrayList<$<Track>>> MediaLibraryProxyProvider::getArtistTopTracks(String artistURI) {
		return Promise<ArrayList<$<Track>>>::reject(std::runtime_error("MediaLibraryProxyProvider cannot fetch artist top tracks"));
	}

	MediaLibraryProxyProvider::ArtistAlbumsGenerator MediaLibraryProxyProvider::getArtistAlbums(String artistURI) {
		throw std::runtime_error("MediaLibraryProxyProvider cannot fetch artist albums");
	}
	
	MediaLibraryProxyProvider::UserPlaylistsGenerator MediaLibraryProxyProvider::getUserPlaylists(String userURI) {
		throw std::runtime_error("MediaLibraryProxyProvider cannot fetch user playlists");
	}
	
	Album::MutatorDelegate* MediaLibraryProxyProvider::createAlbumMutatorDelegate($<Album> album) {
		return nullptr;
	}

	Playlist::MutatorDelegate* MediaLibraryProxyProvider::createPlaylistMutatorDelegate($<Playlist> playlist) {
		return nullptr;
	}

	bool MediaLibraryProxyProvider::hasLibrary() const {
		return false;
	}
	
	MediaLibraryProxyProvider::LibraryItemGenerator MediaLibraryProxyProvider::generateLibrary(GenerateLibraryOptions options) {
		throw std::runtime_error("MediaLibraryProxyProvider cannot generate library");
	}

	Promise<bool> MediaLibraryProxyProvider::isPlaylistEditable($<Playlist> playlist) {
		if(playlist->mediaProvider() == this) {
			return Promise<bool>::resolve(false);
		}
		return playlist->mediaProvider()->isPlaylistEditable(playlist);
	}
	
	MediaPlaybackProvider* MediaLibraryProxyProvider::player() {
		return nullptr;
	}

	const MediaPlaybackProvider* MediaLibraryProxyProvider::player() const {
		return nullptr;
	}

	ArrayList<MediaProvider*> MediaLibraryProxyProvider::getMediaProviders() {
		return mediaProviders;
	}

	void MediaLibraryProxyProvider::addMediaProvider(MediaProvider* mediaProvider) {
		mediaProviders.pushBack(mediaProvider);
	}

	void MediaLibraryProxyProvider::removeMediaProvider(MediaProvider* mediaProvider) {
		mediaProviders.removeLastEqual(mediaProvider);
	}

	MediaProvider* MediaLibraryProxyProvider::getMediaProvider(const String& name) {
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

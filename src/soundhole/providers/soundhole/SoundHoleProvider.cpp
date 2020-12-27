//
//  SoundHoleProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/27/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "SoundHoleProvider.hpp"

namespace sh {
	SoundHoleProvider::SoundHoleProvider() {
		//
	}

	SoundHoleProvider::~SoundHoleProvider() {
		//
	}



	String SoundHoleProvider::name() const {
		return "soundhole";
	}

	String SoundHoleProvider::displayName() const {
		return "SoundHole";
	}



	#pragma mark Login

	Promise<bool> SoundHoleProvider::login() {
		return Promise<bool>::reject(std::logic_error("SoundHoleProvider::login is unimplemented"));
	}

	void SoundHoleProvider::logout() {
		// do nothing
	}

	bool SoundHoleProvider::isLoggedIn() const {
		return false;
	}



	#pragma mark Current User

	Promise<ArrayList<String>> SoundHoleProvider::getCurrentUserIds() {
		return Promise<ArrayList<String>>::resolve({});
	}



	#pragma mark Media Item Fetching

	Promise<Track::Data> SoundHoleProvider::getTrackData(String uri) {
		return Promise<Track::Data>::reject(std::logic_error("SoundHoleProvider::getTrackData is unimplemented"));
	}

	Promise<Artist::Data> SoundHoleProvider::getArtistData(String uri) {
		return Promise<Artist::Data>::reject(std::logic_error("SoundHole provider does not have artists"));
	}

	Promise<Album::Data> SoundHoleProvider::getAlbumData(String uri) {
		return Promise<Album::Data>::reject(std::logic_error("SoundHole provider does not have albums"));
	}

	Promise<Playlist::Data> SoundHoleProvider::getPlaylistData(String uri) {
		return Promise<Playlist::Data>::reject(std::logic_error("SoundHoleProvider::getPlaylistData is unimplemented"));
	}

	Promise<UserAccount::Data> SoundHoleProvider::getUserData(String uri) {
		return Promise<UserAccount::Data>::reject(std::logic_error("SoundHoleProvider::getUserData is unimplemented"));
	}



	Promise<ArrayList<$<Track>>> SoundHoleProvider::getArtistTopTracks(String artistURI) {
		return Promise<ArrayList<$<Track>>>::reject(std::logic_error("SoundHole doesn't have artist top tracks"));
	}

	SoundHoleProvider::ArtistAlbumsGenerator SoundHoleProvider::getArtistAlbums(String artistURI) {
		using YieldResult = typename ArtistAlbumsGenerator::YieldResult;
		return ArtistAlbumsGenerator([=]() {
			return Promise<YieldResult>::reject(std::logic_error("SoundHole does not support artist albums"));
		});
	}

	SoundHoleProvider::UserPlaylistsGenerator SoundHoleProvider::getUserPlaylists(String userURI) {
		using YieldResult = typename UserPlaylistsGenerator::YieldResult;
		return UserPlaylistsGenerator([=]() {
			return Promise<YieldResult>::reject(std::logic_error("SoundHoleProvider::getUserPlaylists is unimplemented"));
		});
	}



	Album::MutatorDelegate* SoundHoleProvider::createAlbumMutatorDelegate($<Album> album) {
		throw std::logic_error("SoundHole does not support albums");
	}

	Playlist::MutatorDelegate* SoundHoleProvider::createPlaylistMutatorDelegate($<Playlist> playlist) {
		throw std::logic_error("SoundHoleProvider::createPlaylistMutatorDelegate is unimplemented");
	}



	#pragma mark User Library

	bool SoundHoleProvider::hasLibrary() const {
		return false;
	}

	SoundHoleProvider::LibraryItemGenerator SoundHoleProvider::generateLibrary(GenerateLibraryOptions options) {
		using YieldResult = typename LibraryItemGenerator::YieldResult;
		return LibraryItemGenerator([=]() {
			return Promise<YieldResult>::reject(std::logic_error("SoundHoleProvider::generateLibrary is unimplemented"));
		});
	}



	#pragma mark Playlists

	bool SoundHoleProvider::canCreatePlaylists() const {
		return true;
	}

	ArrayList<Playlist::Privacy> SoundHoleProvider::supportedPlaylistPrivacies() const {
		return { Playlist::Privacy::PRIVATE, Playlist::Privacy::PUBLIC };
	}

	Promise<$<Playlist>> SoundHoleProvider::createPlaylist(String name, CreatePlaylistOptions options) {
		return Promise<$<Playlist>>::reject(std::logic_error("SoundHoleProvider::createPlaylist is unimplemented"));
	}

	Promise<bool> SoundHoleProvider::isPlaylistEditable($<Playlist> playlist) {
		return Promise<bool>::reject(std::logic_error("SoundHoleProvider::isPlaylistEditable is unimplemented"));
	}



	#pragma mark Player

	MediaPlaybackProvider* SoundHoleProvider::player() {
		return nullptr;
	}

	const MediaPlaybackProvider* SoundHoleProvider::player() const {
		return nullptr;
	}
}

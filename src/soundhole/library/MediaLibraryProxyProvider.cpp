//
//  MediaLibraryProxyProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 5/26/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "MediaLibraryProxyProvider.hpp"
#include "MediaLibrary.hpp"

namespace sh {
	MediaLibraryProxyProvider::MediaLibraryProxyProvider(MediaLibrary* library)
	: _library(library) {
		//
	}

	MediaLibraryProxyProvider::~MediaLibraryProxyProvider() {
		//
	}



	MediaLibrary* MediaLibraryProxyProvider::library() {
		return _library;
	}

	const MediaLibrary* MediaLibraryProxyProvider::library() const {
		return _library;
	}

	MediaDatabase* MediaLibraryProxyProvider::database() {
		return _library->database();
	}

	const MediaDatabase* MediaLibraryProxyProvider::database() const {
		return _library->database();
	}



	String MediaLibraryProxyProvider::name() const {
		return NAME;
	}

	String MediaLibraryProxyProvider::displayName() const {
		return "Media Library";
	}



	#pragma mark Login (non-functional stubs)
	
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



	#pragma mark Media Item Fetching

	Promise<Track::Data> MediaLibraryProxyProvider::getTrackData(String uri) {
		return Promise<Track::Data>::reject(std::runtime_error("MediaLibraryProxyProvider cannot fetch tracks"));
	}

	Promise<Artist::Data> MediaLibraryProxyProvider::getArtistData(String uri) {
		return Promise<Artist::Data>::reject(std::runtime_error("MediaLibraryProxyProvider cannot fetch artists"));
	}

	Promise<Album::Data> MediaLibraryProxyProvider::getAlbumData(String uri) {
		return Promise<Album::Data>::reject(std::runtime_error("MediaLibraryProxyProvider cannot fetch albums"));
	}

	Promise<UserAccount::Data> MediaLibraryProxyProvider::getUserData(String uri) {
		return Promise<UserAccount::Data>::reject(std::runtime_error("MediaLibraryProxyProvider cannot fetch users"));
	}

	Promise<Playlist::Data> MediaLibraryProxyProvider::getPlaylistData(String uri) {
		// TODO get playlists
		return Promise<Playlist::Data>::reject(std::runtime_error("MediaLibraryProxyProvider cannot fetch playlists"));
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



	#pragma mark User Library

	bool MediaLibraryProxyProvider::hasLibrary() const {
		return false;
	}
	
	MediaLibraryProxyProvider::LibraryItemGenerator MediaLibraryProxyProvider::generateLibrary(GenerateLibraryOptions options) {
		throw std::runtime_error("MediaLibraryProxyProvider cannot generate library");
	}

	bool MediaLibraryProxyProvider::canFollowArtists() const {
		// return true to ensure code doesn't attempt to save local media library artists elsewhere successfully
		return true;
	}

	Promise<void> MediaLibraryProxyProvider::followArtist(String artistURI) {
		return Promise<void>::reject(std::runtime_error("MediaLibraryProxyProvider cannot follow artists"));
	}

	Promise<void> MediaLibraryProxyProvider::unfollowArtist(String artistURI) {
		return Promise<void>::reject(std::runtime_error("MediaLibraryProxyProvider cannot unfollow artists"));
	}

	bool MediaLibraryProxyProvider::canFollowUsers() const {
		// return true to ensure code doesn't attempt to save local media library users elsewhere successfully
		return true;
	}

	Promise<void> MediaLibraryProxyProvider::followUser(String userURI) {
		return Promise<void>::reject(std::runtime_error("MediaLibraryProxyProvider cannot follow users"));
	}

	Promise<void> MediaLibraryProxyProvider::unfollowUser(String userURI) {
		return Promise<void>::reject(std::runtime_error("MediaLibraryProxyProvider cannot unfollow users"));
	}

	bool MediaLibraryProxyProvider::canSaveTracks() const {
		// return true to ensure code doesn't attempt to save local media library playlists elsewhere successfully
		return true;
	}

	Promise<void> MediaLibraryProxyProvider::saveTrack(String trackURI) {
		return Promise<void>::reject(std::runtime_error("MediaLibraryProxyProvider cannot save tracks"));
	}

	Promise<void> MediaLibraryProxyProvider::unsaveTrack(String trackURI) {
		return Promise<void>::reject(std::runtime_error("MediaLibraryProxyProvider cannot unsave tracks"));
	}

	bool MediaLibraryProxyProvider::canSaveAlbums() const {
		// return true to ensure code doesn't attempt to save local media library playlists elsewhere successfully
		return true;
	}

	Promise<void> MediaLibraryProxyProvider::saveAlbum(String albumURI) {
		return Promise<void>::reject(std::runtime_error("MediaLibraryProxyProvider cannot save tracks"));
	}

	Promise<void> MediaLibraryProxyProvider::unsaveAlbum(String albumURI) {
		return Promise<void>::reject(std::runtime_error("MediaLibraryProxyProvider cannot unsave tracks"));
	}

	bool MediaLibraryProxyProvider::canSavePlaylists() const {
		// return true to ensure code doesn't attempt to save local media library playlists elsewhere successfully
		return true;
	}

	Promise<void> MediaLibraryProxyProvider::savePlaylist(String playlistURI) {
		return Promise<void>::reject(std::runtime_error("MediaLibraryProxyProvider cannot save playlists"));
	}

	Promise<void> MediaLibraryProxyProvider::unsavePlaylist(String playlistURI) {
		return Promise<void>::reject(std::runtime_error("MediaLibraryProxyProvider cannot unsave playlists"));
	}



	#pragma mark Player
	
	MediaPlaybackProvider* MediaLibraryProxyProvider::player() {
		return nullptr;
	}

	const MediaPlaybackProvider* MediaLibraryProxyProvider::player() const {
		return nullptr;
	}



	#pragma mark Library Tracks Collection

	$<MediaLibraryTracksCollection> MediaLibraryProxyProvider::libraryTracksCollection(const MediaLibraryTracksCollection::Data& data) {
		return MediaLibraryTracksCollection::new$(this, data);
	}

	$<MediaLibraryTracksCollection> MediaLibraryProxyProvider::libraryTracksCollection(const LibraryTracksFilters& filters, Optional<size_t> itemCount, Map<size_t,MediaLibraryTracksCollectionItem::Data> items) {
		return libraryTracksCollection(MediaLibraryTracksCollection::data(filters, itemCount, items));
	}

	Promise<$<MediaLibraryTracksCollection>> MediaLibraryProxyProvider::getLibraryTracksCollection(GetLibraryTracksCollectionOptions options) {
		auto collection = MediaLibraryTracksCollection::new$(this, options.filters);
		return collection->loadItems(options.itemsStartIndex.valueOr(0), options.itemsStartIndex.valueOr(25))
		.map([=]() {
			return collection;
		});
	}



	#pragma mark Playback History Collection

	$<PlaybackHistoryTrackCollection> MediaLibraryProxyProvider::playbackHistoryCollection(const PlaybackHistoryTrackCollection::Data& data) {
		return PlaybackHistoryTrackCollection::new$(this, data);
	}

	$<PlaybackHistoryTrackCollection> MediaLibraryProxyProvider::playbackHistoryCollection(const PlaybackHistoryFilters& filters, Optional<size_t> itemCount, Map<size_t,PlaybackHistoryTrackCollectionItem::Data> items) {
		return playbackHistoryCollection(PlaybackHistoryTrackCollection::data(filters, itemCount, items));
	}

	Promise<$<PlaybackHistoryTrackCollection>> MediaLibraryProxyProvider::getPlaybackHistoryCollection(GetPlaybackHistoryCollectionOptions options) {
		auto collection = PlaybackHistoryTrackCollection::new$(this, options.filters);
		return collection->loadItems(options.itemsStartIndex.valueOr(0), options.itemsStartIndex.valueOr(25))
		.map([=]() {
			return collection;
		});
	}
}

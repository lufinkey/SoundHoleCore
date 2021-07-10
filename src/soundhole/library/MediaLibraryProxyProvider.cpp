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
	: library(library) {
		//
	}

	MediaLibraryProxyProvider::~MediaLibraryProxyProvider() {
		//
	}



	MediaDatabase* MediaLibraryProxyProvider::database() {
		return library->database();
	}

	const MediaDatabase* MediaLibraryProxyProvider::database() const {
		return library->database();
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



	#pragma mark User Library

	bool MediaLibraryProxyProvider::hasLibrary() const {
		return false;
	}
	
	MediaLibraryProxyProvider::LibraryItemGenerator MediaLibraryProxyProvider::generateLibrary(GenerateLibraryOptions options) {
		throw std::runtime_error("MediaLibraryProxyProvider cannot generate library");
	}

	Promise<void> MediaLibraryProxyProvider::followArtist(String artistURI) {
		return Promise<void>::reject(std::runtime_error("MediaLibraryProxyProvider cannot follow artists"));
	}

	Promise<void> MediaLibraryProxyProvider::unfollowArtist(String artistURI) {
		return Promise<void>::reject(std::runtime_error("MediaLibraryProxyProvider cannot unfollow artists"));
	}

	Promise<void> MediaLibraryProxyProvider::followUser(String userURI) {
		return Promise<void>::reject(std::runtime_error("MediaLibraryProxyProvider cannot follow users"));
	}

	Promise<void> MediaLibraryProxyProvider::unfollowUser(String userURI) {
		return Promise<void>::reject(std::runtime_error("MediaLibraryProxyProvider cannot unfollow users"));
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

	Promise<$<MediaLibraryTracksCollection>> MediaLibraryProxyProvider::getLibraryTracksCollection(GetLibraryTracksOptions options) {
		auto db = database();
		size_t startIndex = options.itemsStartIndex.valueOr(0);
		size_t endIndex = startIndex + options.itemsLimit.valueOr(24);
		return db->getSavedTracksJson({
			.startIndex=startIndex,
			.endIndex=endIndex
		}, {
			.libraryProvider = (options.filters.libraryProvider != nullptr) ? options.filters.libraryProvider->name() : String(),
			.orderBy = options.filters.orderBy,
			.order = options.filters.order
		}).map(nullptr, [=](MediaDatabase::GetJsonItemsListResult results) -> $<MediaLibraryTracksCollection> {
			auto items = Map<size_t,MediaLibraryTracksCollectionItem::Data>();
			size_t itemIndex = startIndex;
			for(auto& jsonItem : results.items) {
				auto jsonItemObj = jsonItem.object_items();
				auto trackJsonIt = jsonItemObj.find("mediaItem");
				if(trackJsonIt != jsonItemObj.end()) {
					auto trackJson = trackJsonIt->second;
					jsonItemObj.erase(trackJsonIt);
					jsonItemObj["track"] = trackJson;
				}
				items.insert_or_assign(itemIndex, MediaLibraryTracksCollectionItem::Data::fromJson(jsonItemObj, library->mediaProviderStash));
				itemIndex++;
			}
			return libraryTracksCollection(options.filters, results.total, items);
		});
	}
}

//
//  MediaDatabase.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 5/25/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <string>
#include <map>
#include <soundhole/common.hpp>
#include <soundhole/media/Track.hpp>
#include <soundhole/media/Album.hpp>
#include <soundhole/media/Playlist.hpp>
#include <soundhole/media/MediaProvider.hpp>
#include <soundhole/media/MediaProviderStash.hpp>

struct sqlite3;

namespace sh {
	class SQLiteTransaction;

	class MediaDatabase {
	public:
		struct Options {
			String name;
			String location;
			MediaProviderStash* stash;
		};
		MediaDatabase(Options);
		~MediaDatabase();
		
		void open();
		bool isOpen() const;
		void close();
		
		struct TransactionOptions {
			bool useSQLTransaction = true;
		};
		Promise<std::map<String,LinkedList<Json>>> transaction(Function<void(SQLiteTransaction&)> executor);
		Promise<std::map<String,LinkedList<Json>>> transaction(TransactionOptions options, Function<void(SQLiteTransaction&)> executor);
		
		struct InitializeOptions {
			bool purge = false;
		};
		Promise<void> initialize(InitializeOptions options = InitializeOptions{.purge=false});
		Promise<void> reset();
		
		struct CacheOptions {
			std::map<std::string,std::string> dbState;
		};
		
		struct CacheTracksOptions {
			bool includeAlbums = false;
			std::map<std::string,std::string> dbState;
		};
		Promise<void> cacheTracks(ArrayList<$<Track>> tracks, CacheTracksOptions options = CacheTracksOptions{.includeAlbums=false});
		Promise<LinkedList<Json>> getTracksJson(ArrayList<String> uris);
		Promise<Json> getTrackJson(String uri);
		Promise<size_t> getTrackCount();
		
		Promise<void> cacheTrackCollections(ArrayList<$<TrackCollection>> collections, CacheOptions options = CacheOptions());
		Promise<LinkedList<Json>> getTrackCollectionsJson(ArrayList<String> uris);
		struct GetTrackCollectionJsonOptions {
			Optional<size_t> itemsOffset;
			Optional<size_t> itemsCount;
		};
		Promise<Json> getTrackCollectionJson(String uri, GetTrackCollectionJsonOptions options);
		Promise<std::map<size_t,Json>> getTrackCollectionItemsJson(String collectionURI, size_t offset, size_t count);
		
		Promise<void> cacheArtists(ArrayList<$<Artist>> artists, CacheOptions options = CacheOptions());
		Promise<LinkedList<Json>> getArtistsJson(ArrayList<String> uris);
		Promise<Json> getArtistJson(String uri);
		
		Promise<void> cacheLibraryItems(ArrayList<MediaProvider::LibraryItem> items, CacheOptions options = CacheOptions());
		
		struct GetSavedTracksCountOptions {
			String libraryProvider;
		};
		Promise<size_t> getSavedTracksCount(GetSavedTracksCountOptions options = GetSavedTracksCountOptions());
		struct GetSavedTracksJsonOptions {
			String libraryProvider;
		};
		Promise<LinkedList<Json>> getSavedTracksJson(size_t offset, size_t count, GetSavedTracksJsonOptions options = GetSavedTracksJsonOptions());
		
		struct GetSavedAlbumsCountOptions {
			String libraryProvider;
		};
		Promise<size_t> getSavedAlbumsCount(GetSavedAlbumsCountOptions options = GetSavedAlbumsCountOptions());
		struct GetSavedAlbumsJsonOptions {
			String libraryProvider;
		};
		Promise<LinkedList<Json>> getSavedAlbumsJson(size_t offset, size_t count, GetSavedAlbumsJsonOptions options = GetSavedAlbumsJsonOptions());
		
		struct GetSavedPlaylistsCountOptions {
			String libraryProvider;
		};
		Promise<size_t> getSavedPlaylistsCount(GetSavedPlaylistsCountOptions options = GetSavedPlaylistsCountOptions());
		struct GetSavedPlaylistsJsonOptions {
			String libraryProvider;
		};
		Promise<LinkedList<Json>> getSavedPlaylistsJson(size_t offset, size_t count, GetSavedPlaylistsJsonOptions options = GetSavedPlaylistsJsonOptions());
		
	private:
		void applyDBState(SQLiteTransaction& tx, std::map<std::string,std::string> state);
		
		Json transformDBTrack(Json json);
		Json transformDBTrackCollection(Json json);
		
		Options options;
		sqlite3* db;
		DispatchQueue* queue;
	};
}

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
#include "SQLIndexRange.hpp"
#include "SQLOrder.hpp"
#include "SQLOrderBy.hpp"

struct sqlite3;

namespace sh {
	class SQLiteTransaction;

	class MediaDatabase {
	public:
		struct Options {
			String path;
			MediaProviderStash* stash;
		};
		
		MediaDatabase(Options);
		~MediaDatabase();
		
		MediaProviderStash* getProviderStash();
		const MediaProviderStash* getProviderStash() const;
		
		void open();
		bool isOpen() const;
		void close();
		
		struct TransactionOptions {
			bool useSQLTransaction = true;
		};
		Promise<std::map<String,LinkedList<Json>>> transaction(TransactionOptions options, Function<void(SQLiteTransaction&)> executor);
		
		struct InitializeOptions {
			bool purge = false;
		};
		Promise<void> initialize(InitializeOptions options = InitializeOptions{.purge=false});
		Promise<void> reset();
		
		struct CacheOptions {
			std::map<String,String> dbState;
		};
		
		struct GetJsonItemsListResult {
			LinkedList<Json> items;
			size_t total;
		};
		
		Promise<void> cacheTracks(ArrayList<$<Track>> tracks, CacheOptions options = CacheOptions());
		Promise<LinkedList<Json>> getTracksJson(ArrayList<String> uris);
		Promise<Json> getTrackJson(String uri);
		Promise<size_t> getTrackCount();
		
		Promise<void> cacheTrackCollections(ArrayList<$<TrackCollection>> collections, CacheOptions options = CacheOptions());
		Promise<LinkedList<Json>> getTrackCollectionsJson(ArrayList<String> uris);
		Promise<Json> getTrackCollectionJson(String uri, Optional<sql::IndexRange> itemsRange = std::nullopt);
		
		Promise<void> cacheTrackCollectionItems($<TrackCollection> collection, Optional<sql::IndexRange> itemsRange = std::nullopt, CacheOptions options = CacheOptions());
		Promise<void> updateTrackCollectionVersionId($<TrackCollection> collection, CacheOptions options = CacheOptions());
		Promise<std::map<size_t,Json>> getTrackCollectionItemsJson(String collectionURI, sql::IndexRange range);
		
		Promise<void> cacheArtists(ArrayList<$<Artist>> artists, CacheOptions options = CacheOptions());
		Promise<LinkedList<Json>> getArtistsJson(ArrayList<String> uris);
		Promise<Json> getArtistJson(String uri);
		
		Promise<void> cacheLibraryItems(ArrayList<MediaProvider::LibraryItem> items, CacheOptions options = CacheOptions());
		
		struct GetSavedItemsCountOptions {
			String libraryProvider;
		};
		
		Promise<size_t> getSavedTracksCount(GetSavedItemsCountOptions options = GetSavedItemsCountOptions());
		struct GetSavedTracksJsonOptions {
			String libraryProvider;
			sql::LibraryItemOrderBy orderBy = sql::LibraryItemOrderBy::ADDED_AT;
			sql::Order order = sql::Order::DESC;
		};
		Promise<GetJsonItemsListResult> getSavedTracksJson(sql::IndexRange range, GetSavedTracksJsonOptions options = GetSavedTracksJsonOptions{
			.orderBy=sql::LibraryItemOrderBy::ADDED_AT,
			.order=sql::Order::DESC});
		Promise<Json> getSavedTrackJson(String uri);
		Promise<ArrayList<bool>> hasSavedTracks(ArrayList<String> uris);
		
		Promise<size_t> getSavedAlbumsCount(GetSavedItemsCountOptions options = GetSavedItemsCountOptions());
		struct GetSavedAlbumsJsonOptions {
			String libraryProvider;
			Optional<sql::IndexRange> range;
			sql::LibraryItemOrderBy orderBy = sql::LibraryItemOrderBy::NAME;
			sql::Order order = sql::Order::ASC;
		};
		Promise<GetJsonItemsListResult> getSavedAlbumsJson(GetSavedAlbumsJsonOptions options = GetSavedAlbumsJsonOptions{
			.orderBy=sql::LibraryItemOrderBy::NAME,
			.order=sql::Order::DESC});
		Promise<Json> getSavedAlbumJson(String uri);
		Promise<ArrayList<bool>> hasSavedAlbums(ArrayList<String> uris);
		
		Promise<size_t> getSavedPlaylistsCount(GetSavedItemsCountOptions options = GetSavedItemsCountOptions());
		struct GetSavedPlaylistsJsonOptions {
			String libraryProvider;
			Optional<sql::IndexRange> range;
			sql::LibraryItemOrderBy orderBy = sql::LibraryItemOrderBy::NAME;
			sql::Order order = sql::Order::ASC;
		};
		Promise<GetJsonItemsListResult> getSavedPlaylistsJson(GetSavedPlaylistsJsonOptions options = GetSavedPlaylistsJsonOptions{
			.orderBy=sql::LibraryItemOrderBy::NAME,
			.order=sql::Order::ASC});
		Promise<Json> getSavedPlaylistJson(String uri);
		Promise<ArrayList<bool>> hasSavedPlaylists(ArrayList<String> uris);
		
		Promise<size_t> getFollowedArtistsCount(GetSavedItemsCountOptions options = GetSavedItemsCountOptions());
		struct GetFollowedArtistsJsonOptions {
			String libraryProvider;
			Optional<sql::IndexRange> range;
			sql::LibraryItemOrderBy orderBy = sql::LibraryItemOrderBy::NAME;
			sql::Order order = sql::Order::ASC;
		};
		Promise<GetJsonItemsListResult> getFollowedArtistsJson(GetFollowedArtistsJsonOptions options = GetFollowedArtistsJsonOptions{
			.orderBy=sql::LibraryItemOrderBy::NAME,
			.order=sql::Order::ASC});
		Promise<Json> getFollowedArtistJson(String uri);
		Promise<ArrayList<bool>> hasFollowedArtists(ArrayList<String> uris);
		
		Promise<size_t> getFollowedUserAccountsCount(GetSavedItemsCountOptions options = GetSavedItemsCountOptions());
		struct GetFollowedUserAccountsJsonOptions {
			String libraryProvider;
			Optional<sql::IndexRange> range;
			sql::LibraryItemOrderBy orderBy = sql::LibraryItemOrderBy::NAME;
			sql::Order order = sql::Order::ASC;
		};
		Promise<GetJsonItemsListResult> getFollowedUserAccountsJson(GetFollowedUserAccountsJsonOptions options = GetFollowedUserAccountsJsonOptions{
			.orderBy=sql::LibraryItemOrderBy::NAME,
			.order=sql::Order::ASC});
		Promise<Json> getFollowedUserAccountJson(String uri);
		Promise<ArrayList<bool>> hasFollowedUserAccounts(ArrayList<String> uris);
		
		Promise<void> setState(std::map<String,String> state);
		Promise<std::map<String,String>> getState(ArrayList<String> keys);
		Promise<String> getStateValue(String key, String defaultValue);
		
		Promise<void> loadAlbumItems($<Album> album, Album::MutatorDelegate::Mutator* mutator, size_t index, size_t count);
		Promise<void> loadPlaylistItems($<Playlist> playlist, Playlist::MutatorDelegate::Mutator* mutator, size_t index, size_t count);
		
		
		Promise<void> deleteSavedTrack(String uri);
		Promise<void> deleteSavedAlbum(String uri);
		Promise<void> deleteSavedPlaylist(String uri);
		Promise<void> deleteFollowedArtist(String uri);
		Promise<void> deleteFollowedUserAccount(String uri);
		
	private:
		static void applyDBState(SQLiteTransaction& tx, std::map<String,String> state);
		
		Options options;
		std::recursive_mutex dbMutex;
		sqlite3* db;
		DispatchQueue* queue;
	};
}

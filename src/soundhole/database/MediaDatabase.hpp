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
#include <soundhole/media/PlaybackHistoryItem.hpp>
#include <soundhole/media/Scrobble.hpp>
#include <soundhole/media/UnmatchedScrobble.hpp>
#include <soundhole/media/MediaProviderStash.hpp>
#include <soundhole/media/ScrobblerStash.hpp>
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
			MediaProviderStash* mediaProviderStash;
			ScrobblerStash* scrobblerStash;
		};
		
		MediaDatabase(Options);
		~MediaDatabase();
		
		MediaProviderStash* mediaProviderStash();
		const MediaProviderStash* mediaProviderStash() const;
		
		ScrobblerStash* scrobblerStash();
		const ScrobblerStash* scrobblerStash() const;
		
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
			
			bool empty() const;
			void apply(SQLiteTransaction& tx) const;
		};
		
		struct GetJsonItemsListResult {
			LinkedList<Json> items;
			size_t total;
		};
		
		template<typename T>
		struct GetItemsListResult {
			ArrayList<T> items;
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
		struct GetSavedTracksOptions {
			String libraryProvider;
			sql::LibraryItemOrderBy orderBy = sql::LibraryItemOrderBy::ADDED_AT;
			sql::Order order = sql::Order::DESC;
		};
		Promise<GetJsonItemsListResult> getSavedTracksJson(sql::IndexRange range, GetSavedTracksOptions options = GetSavedTracksOptions{
			.orderBy=sql::LibraryItemOrderBy::ADDED_AT,
			.order=sql::Order::DESC});
		Promise<Json> getSavedTrackJson(String uri);
		Promise<ArrayList<bool>> hasSavedTracks(ArrayList<String> uris);
		Promise<void> deleteSavedTrack(String uri);
		
		
		Promise<size_t> getSavedAlbumsCount(GetSavedItemsCountOptions options = GetSavedItemsCountOptions());
		struct GetSavedAlbumsOptions {
			String libraryProvider;
			Optional<sql::IndexRange> range;
			sql::LibraryItemOrderBy orderBy = sql::LibraryItemOrderBy::NAME;
			sql::Order order = sql::Order::ASC;
		};
		Promise<GetJsonItemsListResult> getSavedAlbumsJson(GetSavedAlbumsOptions options = GetSavedAlbumsOptions{
			.orderBy=sql::LibraryItemOrderBy::NAME,
			.order=sql::Order::DESC});
		Promise<Json> getSavedAlbumJson(String uri);
		Promise<ArrayList<bool>> hasSavedAlbums(ArrayList<String> uris);
		Promise<void> deleteSavedAlbum(String uri);
		
		
		Promise<size_t> getSavedPlaylistsCount(GetSavedItemsCountOptions options = GetSavedItemsCountOptions());
		struct GetSavedPlaylistsOptions {
			String libraryProvider;
			Optional<sql::IndexRange> range;
			sql::LibraryItemOrderBy orderBy = sql::LibraryItemOrderBy::NAME;
			sql::Order order = sql::Order::ASC;
		};
		Promise<GetJsonItemsListResult> getSavedPlaylistsJson(GetSavedPlaylistsOptions options = GetSavedPlaylistsOptions{
			.orderBy=sql::LibraryItemOrderBy::NAME,
			.order=sql::Order::ASC});
		Promise<Json> getSavedPlaylistJson(String uri);
		Promise<ArrayList<bool>> hasSavedPlaylists(ArrayList<String> uris);
		Promise<void> deleteSavedPlaylist(String uri);
		
		
		Promise<size_t> getFollowedArtistsCount(GetSavedItemsCountOptions options = GetSavedItemsCountOptions());
		struct GetFollowedArtistsOptions {
			String libraryProvider;
			Optional<sql::IndexRange> range;
			sql::LibraryItemOrderBy orderBy = sql::LibraryItemOrderBy::NAME;
			sql::Order order = sql::Order::ASC;
		};
		Promise<GetJsonItemsListResult> getFollowedArtistsJson(GetFollowedArtistsOptions options = GetFollowedArtistsOptions{
			.orderBy=sql::LibraryItemOrderBy::NAME,
			.order=sql::Order::ASC});
		Promise<Json> getFollowedArtistJson(String uri);
		Promise<ArrayList<bool>> hasFollowedArtists(ArrayList<String> uris);
		Promise<void> deleteFollowedArtist(String uri);
		
		
		Promise<size_t> getFollowedUserAccountsCount(GetSavedItemsCountOptions options = GetSavedItemsCountOptions());
		struct GetFollowedUserAccountsOptions {
			String libraryProvider;
			Optional<sql::IndexRange> range;
			sql::LibraryItemOrderBy orderBy = sql::LibraryItemOrderBy::NAME;
			sql::Order order = sql::Order::ASC;
		};
		Promise<GetJsonItemsListResult> getFollowedUserAccountsJson(GetFollowedUserAccountsOptions options = GetFollowedUserAccountsOptions{
			.orderBy = sql::LibraryItemOrderBy::NAME,
			.order = sql::Order::ASC});
		Promise<Json> getFollowedUserAccountJson(String uri);
		Promise<ArrayList<bool>> hasFollowedUserAccounts(ArrayList<String> uris);
		Promise<void> deleteFollowedUserAccount(String uri);
		
		
		Promise<void> cachePlaybackHistoryItems(ArrayList<$<PlaybackHistoryItem>> historyItems, CacheOptions options = CacheOptions());
		struct PlaybackHistoryItemFilters {
			String provider;
			ArrayList<String> trackURIs;
			Optional<Date> minDate;
			bool minDateInclusive = true;
			Optional<Date> maxDate;
			bool maxDateInclusive = false;
			Optional<double> minDuration;
			Optional<double> minDurationRatio;
			Optional<bool> includeNullDuration;
			Optional<PlaybackHistoryItem::Visibility> visibility;
			ArrayList<String> scrobbledBy;
			ArrayList<String> notScrobbledBy;
		};
		Promise<size_t> getPlaybackHistoryItemCount(PlaybackHistoryItemFilters filters = PlaybackHistoryItemFilters{
			.minDateInclusive = true,
			.maxDateInclusive = false
		});
		struct GetPlaybackHistoryItemsOptions {
			PlaybackHistoryItemFilters filters;
			Optional<sql::IndexRange> range;
			sql::Order order = sql::Order::DESC;
		};
		Promise<GetJsonItemsListResult> getPlaybackHistoryItemsJson(GetPlaybackHistoryItemsOptions options = GetPlaybackHistoryItemsOptions{
			.filters = PlaybackHistoryItemFilters{
				.minDateInclusive = true,
				.maxDateInclusive = false
			},
			.order = sql::Order::DESC
		});
		Promise<void> deletePlaybackHistoryItem(Date startTime, String trackURI);
		
		
		Promise<void> cacheScrobbles(ArrayList<$<Scrobble>> scrobbles, CacheOptions options = CacheOptions());
		struct ScrobbleFilters {
			String scrobbler;
			ArrayList<Date> startTimes;
			Optional<Date> minStartTime;
			bool minStartTimeInclusive = true;
			Optional<Date> maxStartTime;
			bool maxStartTimeInclusive = false;
			Optional<bool> uploaded;
		};
		Promise<size_t> getScrobbleCount(ScrobbleFilters filters = ScrobbleFilters{
			.minStartTimeInclusive = true,
			.maxStartTimeInclusive = false
		});
		struct GetScrobblesOptions {
			ScrobbleFilters filters;
			Optional<sql::IndexRange> range;
			sql::Order order = sql::Order::DESC;
		};
		Promise<GetJsonItemsListResult> getScrobblesJson(GetScrobblesOptions options = GetScrobblesOptions{
			.filters = ScrobbleFilters{
				.minStartTimeInclusive = true,
				.maxStartTimeInclusive = false
			},
			.order = sql::Order::DESC
		});
		Promise<GetItemsListResult<$<Scrobble>>> getScrobbles(GetScrobblesOptions options = GetScrobblesOptions{
			.filters = ScrobbleFilters{
				.minStartTimeInclusive = true,
				.maxStartTimeInclusive = false
			},
			.order = sql::Order::DESC
		});
		Promise<ArrayList<bool>> hasMatchingScrobbles(ArrayList<$<Scrobble>> scrobbles);
		Promise<void> deleteScrobble(String localID);
		
		
		Promise<void> cacheUnmatchedScrobbles(ArrayList<UnmatchedScrobble> scrobbles, CacheOptions options = CacheOptions());
		struct UnmatchedScrobbleFilters {
			String scrobbler;
			ArrayList<Date> startTimes;
			Optional<Date> minStartTime;
			bool minStartTimeInclusive = true;
			Optional<Date> maxStartTime;
			bool maxStartTimeInclusive = false;
		};
		Promise<size_t> getUnmatchedScrobbleCount(UnmatchedScrobbleFilters filters = UnmatchedScrobbleFilters{
			.minStartTimeInclusive = true,
			.maxStartTimeInclusive = false
		});
		struct GetUnmatchedScrobblesOptions {
			UnmatchedScrobbleFilters filters;
			Optional<sql::IndexRange> range;
			sql::Order order = sql::Order::ASC;
		};
		Promise<GetJsonItemsListResult> getUnmatchedScrobblesJson(GetUnmatchedScrobblesOptions options = GetUnmatchedScrobblesOptions{
			.filters = UnmatchedScrobbleFilters{
				.minStartTimeInclusive = true,
				.maxStartTimeInclusive = false
			},
			.order = sql::Order::ASC
		});
		Promise<GetItemsListResult<UnmatchedScrobble>> getUnmatchedScrobbles(GetUnmatchedScrobblesOptions options = GetUnmatchedScrobblesOptions{
			.filters = UnmatchedScrobbleFilters{
				.minStartTimeInclusive = true,
				.maxStartTimeInclusive = false
			},
			.order = sql::Order::ASC
		});
		Promise<void> replaceUnmatchedScrobbles(ArrayList<Tuple<UnmatchedScrobble,$<Scrobble>>> scrobbleTuples, CacheOptions options = CacheOptions());
		Promise<void> deleteUnmatchedScrobbles(ArrayList<UnmatchedScrobble> scrobbles, CacheOptions options = CacheOptions());
		
		
		Promise<void> loadAlbumItems($<Album> album, Album::Mutator* mutator, size_t index, size_t count);
		Promise<void> loadPlaylistItems($<Playlist> playlist, Playlist::Mutator* mutator, size_t index, size_t count);
		
		
		Promise<void> setState(std::map<String,String> state);
		Promise<std::map<String,String>> getState(ArrayList<String> keys);
		Promise<String> getStateValue(String key, String defaultValue);
		
		/// The DBState key used for resuming syncing a provider's library, without having to resync the entire library
		String stateKey_syncLibraryResumeData(const MediaProvider*) const;
		/// The DBState key used for the last date a scrobbler has matched against the user's playback history
		String stateKey_scrobblerMatchHistoryDate(const Scrobbler*) const;
		
	private:
		static void applyDBState(SQLiteTransaction& tx, std::map<String,String> state);
		
		Options options;
		std::recursive_mutex dbMutex;
		sqlite3* db;
		DispatchQueue* queue;
	};
}

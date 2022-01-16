//
//  MediaDatabaseSQLOperations.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 5/25/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "MediaDatabaseSQL.hpp"
#include "SQLiteTransaction.hpp"
#include "SQLIndexRange.hpp"
#include "SQLOrder.hpp"
#include "SQLOrderBy.hpp"

namespace sh::sql {

void insertOrReplaceArtists(SQLiteTransaction& tx, const ArrayList<$<Artist>>& artists);
void insertOrReplaceFollowedArtists(SQLiteTransaction& tx, const ArrayList<FollowedArtist>& followedArtists);
void insertOrReplaceTracks(SQLiteTransaction& tx, const ArrayList<$<Track>>& tracks, bool includeAlbums);
void insertOrReplaceTrackCollections(SQLiteTransaction& tx, const ArrayList<$<TrackCollection>>& collections);
struct InsertTrackCollectionItemsOptions {
	Optional<IndexRange> range;
	bool includeTrackAlbums = false;
};
void insertOrReplaceItemsFromTrackCollection(SQLiteTransaction& tx, $<TrackCollection> collection, InsertTrackCollectionItemsOptions options = InsertTrackCollectionItemsOptions());
void insertOrReplaceLibraryItems(SQLiteTransaction& tx, const ArrayList<MediaProvider::LibraryItem>& items);
void insertOrReplaceSavedTracks(SQLiteTransaction& tx, const ArrayList<SavedTrack>& savedTracks);
void insertOrReplaceSavedAlbums(SQLiteTransaction& tx, const ArrayList<SavedAlbum>& savedAlbums);
void insertOrReplaceSavedPlaylists(SQLiteTransaction& tx, const ArrayList<SavedPlaylist>& savedPlaylists);
void insertOrReplacePlaybackHistoryItems(SQLiteTransaction& tx, const ArrayList<$<PlaybackHistoryItem>>& items);
void insertOrReplaceScrobbles(SQLiteTransaction& tx, const ArrayList<$<Scrobble>>& scrobbles);
void insertOrReplaceDBStates(SQLiteTransaction& tx, const ArrayList<DBState>& states);
void applyDBState(SQLiteTransaction& tx, std::map<String,String> state);


void selectTrack(SQLiteTransaction& tx, String outKey, String uri);
void selectTrackCount(SQLiteTransaction& tx, String outKey);
void selectTrackCollectionWithOwner(SQLiteTransaction& tx, String outKey, String uri);
void selectTrackCollectionItemsWithTracks(SQLiteTransaction& tx, String outKey, String collectionURI, Optional<IndexRange> range);
void selectArtist(SQLiteTransaction& tx, String outKey, String uri);

struct LibraryItemSelectOptions {
	String libraryProvider;
	Optional<IndexRange> range;
	Order order = Order::DEFAULT;
	LibraryItemOrderBy orderBy = LibraryItemOrderBy::ADDED_AT;
};
void selectSavedTracksWithTracks(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options = LibraryItemSelectOptions());
void selectSavedTrackCount(SQLiteTransaction& tx, String outKey, String libraryProvider = String());
void selectSavedTrack(SQLiteTransaction& tx, String outKey, String uri);
void selectSavedTrackWithTrack(SQLiteTransaction& tx, String outKey, String uri);

void selectSavedAlbumsWithAlbums(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options = LibraryItemSelectOptions());
void selectSavedAlbumCount(SQLiteTransaction& tx, String outKey, String libraryProvider = String());
void selectSavedAlbum(SQLiteTransaction& tx, String outKey, String uri);
void selectSavedAlbumWithAlbum(SQLiteTransaction& tx, String outKey, String uri);

void selectSavedPlaylistsWithPlaylistsAndOwners(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options = LibraryItemSelectOptions());
void selectSavedPlaylistCount(SQLiteTransaction& tx, String outKey, String libraryProvider = String());
void selectSavedPlaylist(SQLiteTransaction& tx, String outKey, String uri);
void selectSavedPlaylistWithPlaylistAndOwner(SQLiteTransaction& tx, String outKey, String uri);

void selectFollowedArtistsWithArtists(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options = LibraryItemSelectOptions());
void selectFollowedArtistCount(SQLiteTransaction& tx, String outKey, String libraryProvider = String());
void selectFollowedArtist(SQLiteTransaction& tx, String outKey, String uri);
void selectFollowedArtistWithArtist(SQLiteTransaction& tx, String outKey, String uri);

void selectFollowedUserAccountsWithUserAccounts(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options = LibraryItemSelectOptions());
void selectFollowedUserAccountCount(SQLiteTransaction& tx, String outKey, String libraryProvider = String());
void selectFollowedUserAccount(SQLiteTransaction& tx, String outKey, String uri);
void selectFollowedUserAccountWithUserAccount(SQLiteTransaction& tx, String outKey, String uri);

void selectLibraryArtists(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options = LibraryItemSelectOptions{.orderBy=LibraryItemOrderBy::NAME});
void selectLibraryArtistCount(SQLiteTransaction& tx, String outKey, String libraryProvider = String());

struct PlaybackHistorySelectFilters {
	String provider;
	ArrayList<String> trackURIs;
	Optional<Date> minDate;
	bool minDateInclusive = true;
	Optional<Date> maxDate;
	bool maxDateInclusive = false;
	Optional<double> minDuration;
	Optional<double> minDurationRatio;
	Optional<bool> includeNullDuration;
	
	String sql(LinkedList<Any>& params) const;
};
struct PlaybackHistorySelectOptions {
	Optional<IndexRange> range;
	Order order = Order::DEFAULT;
};
void selectPlaybackHistoryItemsWithTracks(SQLiteTransaction& tx, String outKey, const PlaybackHistorySelectFilters& filters, const PlaybackHistorySelectOptions& options);
void selectPlaybackHistoryItemCount(SQLiteTransaction& tx, String outKey, const PlaybackHistorySelectFilters& filters);

void selectScrobble(SQLiteTransaction& tx, String outKey, String localID);
struct ScrobbleSelectFilters {
	String scrobbler;
	ArrayList<Date> startTimes;
	Optional<Date> minStartTime;
	bool minStartTimeInclusive = true;
	Optional<Date> maxStartTime;
	bool maxStartTimeInclusive = false;
	Optional<bool> uploaded;
	
	String sql(LinkedList<Any>& params) const;
};
struct ScrobbleSelectOptions {
	Optional<IndexRange> range;
	Order order = Order::DEFAULT;
};
void selectScrobbles(SQLiteTransaction& tx, String outKey, const ScrobbleSelectFilters& filters, const ScrobbleSelectOptions& options);
void selectScrobbleCount(SQLiteTransaction& tx, String outKey, const ScrobbleSelectFilters& filters);

void selectDBState(SQLiteTransaction& tx, String outKey, String stateKey);


void updateTrackCollectionVersionId(SQLiteTransaction& tx, String collectionURI, String versionId);


void deleteSavedTrack(SQLiteTransaction& tx, String trackURI);
void deleteSavedAlbum(SQLiteTransaction& tx, String albumURI);
void deleteSavedPlaylist(SQLiteTransaction& tx, String playlistURI);
void deleteFollowedArtist(SQLiteTransaction& tx, String artistURI);
void deleteFollowedUserAccount(SQLiteTransaction& tx, String userURI);
void deletePlaybackHistoryItem(SQLiteTransaction& tx, Date startTime, String trackURI);
void deleteScrobble(SQLiteTransaction& tx, String localID);
void deleteScrobbles(SQLiteTransaction& tx, const ScrobbleSelectFilters& filters);

void deleteUnreferencedCollectionItems(SQLiteTransaction& tx);
void deleteUnreferencedTracks(SQLiteTransaction& tx);
void deleteUnreferencedCollections(SQLiteTransaction& tx);
void deleteUnreferencedArtists(SQLiteTransaction& tx);
void deleteUnreferencedUserAccounts(SQLiteTransaction& tx);
void deleteUnreferencedScrobbles(SQLiteTransaction& tx);

}

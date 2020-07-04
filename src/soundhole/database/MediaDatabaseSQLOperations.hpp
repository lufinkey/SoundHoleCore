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
void insertOrReplaceDBStates(SQLiteTransaction& tx, const ArrayList<DBState>& states);

void selectTrack(SQLiteTransaction& tx, String outKey, String uri);
void selectTrackCount(SQLiteTransaction& tx, String outKey);
void selectTrackCollection(SQLiteTransaction& tx, String outKey, String uri);
void selectTrackCollectionItemsAndTracks(SQLiteTransaction& tx, String outKey, String collectionURI, Optional<IndexRange> range);
void selectArtist(SQLiteTransaction& tx, String outKey, String uri);
struct LibraryItemSelectOptions {
	String libraryProvider;
	Optional<IndexRange> range;
	Order order = Order::NONE;
	LibraryItemOrderBy orderBy = LibraryItemOrderBy::ADDED_AT;
};
void selectSavedTracksAndTracks(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options = LibraryItemSelectOptions());
void selectSavedTrackCount(SQLiteTransaction& tx, String outKey, String libraryProvider = String());
void selectSavedAlbumsAndAlbums(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options = LibraryItemSelectOptions());
void selectSavedAlbumCount(SQLiteTransaction& tx, String outKey, String libraryProvider = String());
void selectSavedPlaylistsAndPlaylists(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options = LibraryItemSelectOptions());
void selectSavedPlaylistCount(SQLiteTransaction& tx, String outKey, String libraryProvider = String());
void selectLibraryArtists(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options = LibraryItemSelectOptions{.orderBy=LibraryItemOrderBy::NAME});
void selectLibraryArtistCount(SQLiteTransaction& tx, String outKey, String libraryProvider = String());
void selectDBState(SQLiteTransaction& tx, String outKey, String stateKey);

void updateTrackCollectionVersionId(SQLiteTransaction& tx, String collectionURI, String versionId);

void deleteNonLibraryCollectionItems(SQLiteTransaction& tx);
void deleteNonLibraryTracks(SQLiteTransaction& tx);
void deleteNonLibraryCollections(SQLiteTransaction& tx);
void deleteNonLibraryArtists(SQLiteTransaction& tx);

}

//
//  MediaDatabaseSQLOperations.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 5/25/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "MediaDatabaseSQLOperations.hpp"
#include "MediaDatabaseSQLTransformations.hpp"

namespace sh::sql {

struct JoinTable {
	String name;
	String prefix;
	ArrayList<String> columns;
};

String joinedTableColumns(ArrayList<JoinTable> tables) {
	return String::join(tables.reduce(LinkedList<String>{}, [](auto list, auto& table) {
		for(auto& column : table.columns) {
			list.pushBack(String::join({ table.name,".",column," as ",table.prefix,column }));
		}
		return list;
	}), ", ");
}

ArrayList<Json> splitJoinedResults(ArrayList<JoinTable> tables, Json row) {
	ArrayList<Json> splitRows;
	splitRows.reserve(tables.size());
	for(auto& table : tables) {
		Json::object tableRow;
		for(auto& column : table.columns) {
			auto entry = row[table.prefix+column];
			if(!entry.is_null()) {
				tableRow[column] = entry;
			}
		}
		splitRows.pushBack(tableRow);
	}
	return splitRows;
}



String sqlOffsetAndLimitFromRange(Optional<sql::IndexRange> range, LinkedList<Any>& params) {
	if(!range) {
		return String();
	}
	if(range->endIndex == (size_t)-1) {
		if(range->startIndex == 0) {
			return String();
		}
		return String::join({" OFFSET ",sqlParam(params, range->startIndex)});
	}
	return String::join({
		" LIMIT ",sqlParam(params, (range->endIndex - range->startIndex)),
		" OFFSET ",sqlParam(params, range->startIndex)
	});
}



void insertOrReplaceArtists(SQLiteTransaction& tx, const ArrayList<$<Artist>>& artists) {
	if(artists.size() == 0) {
		return;
	}
	LinkedList<Any> fullArtistParams;
	LinkedList<String> fullArtistTuples;
	LinkedList<Any> partialArtistParams;
	LinkedList<String> partialArtistTuples;
	for(auto& artist : artists) {
		if(artist->needsData()) {
			partialArtistTuples.pushBack(artistTuple(partialArtistParams, artist, {.coalesce=true}));
		} else {
			fullArtistTuples.pushBack(artistTuple(fullArtistParams, artist, {.coalesce=false}));
		}
	}
	if(fullArtistTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO Artist (",
			String::join(artistTupleColumns(), ", "),
			") VALUES ",
			String::join(fullArtistTuples, ", ")
		}), fullArtistParams);
	}
	if(partialArtistTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO Artist (",
			String::join(artistTupleColumns(), ", "),
			") VALUES ",
			String::join(partialArtistTuples, ", ")
		}), partialArtistParams);
	}
}

void insertOrReplaceFollowedArtists(SQLiteTransaction& tx, const ArrayList<FollowedArtist>& followedArtists) {
	if(followedArtists.size() == 0) {
		return;
	}
	LinkedList<Any> followedArtistParams;
	LinkedList<String> followedArtistTuples;
	for(auto& followedArtist : followedArtists) {
		followedArtistTuples.pushBack(followedArtistTuple(followedArtistParams, followedArtist));
	}
	tx.addSQL(String::join({
		"INSERT OR REPLACE INTO FollowedArtist (",
		String::join(followedArtistTupleColumns(), ", "),
		") VALUES ",
		String::join(followedArtistTuples, ", ")
	}), followedArtistParams);
}

struct TrackTuplesAndParams {
	LinkedList<String> trackTuples;
	LinkedList<Any> trackParams;
	LinkedList<String> fullArtistTuples;
	LinkedList<Any> fullArtistParams;
	LinkedList<String> partialArtistTuples;
	LinkedList<Any> partialArtistParams;
	LinkedList<String> trackArtistTuples;
	LinkedList<Any> trackArtistParams;
	LinkedList<String> albumTuples;
	LinkedList<Any> albumParams;
	LinkedList<String> albumItemTuples;
	LinkedList<Any> albumItemParams;
};

void addTrackTuples($<Track> track, TrackTuplesAndParams& tuples, bool includeAlbum) {
	tuples.trackTuples.pushBack(trackTuple(tuples.trackParams, track, {.coalesce=true}));
	for(auto& artist : track->artists()) {
		if(artist->uri().empty()) {
			continue;
		}
		if(artist->needsData()) {
			tuples.partialArtistTuples.pushBack(artistTuple(tuples.partialArtistParams, artist, {.coalesce=true}));
		} else {
			tuples.fullArtistTuples.pushBack(artistTuple(tuples.fullArtistParams, artist, {.coalesce=false}));
		}
		tuples.trackArtistTuples.pushBack(trackArtistTuple(tuples.trackArtistParams, {
			.trackURI=track->uri(),
			.artistURI=artist->uri()
		}));
	}
	if(includeAlbum) {
		if(!track->albumURI().empty()) {
			tuples.albumTuples.pushBack(albumTupleFromTrack(tuples.albumParams, track, {.coalesce=true}));
			if(track->trackNumber().value_or(0) >= 1) {
				tuples.albumItemTuples.pushBack(albumItemTupleFromTrack(tuples.albumItemParams, track));
			}
		}
	}
}

void applyTrackTuples(SQLiteTransaction& tx, TrackTuplesAndParams& tuples) {
	if(tuples.fullArtistTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO Artist (",
			String::join(artistTupleColumns(), ", "),
			") VALUES ",
			String::join(tuples.fullArtistTuples, ", ")
		}), tuples.fullArtistParams);
	}
	if(tuples.partialArtistTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO Artist (",
			String::join(artistTupleColumns(), ", "),
			") VALUES ",
			String::join(tuples.partialArtistTuples, ", ")
		}), tuples.partialArtistParams);
	}
	if(tuples.albumTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO TrackCollection (",
			String::join(albumTupleFromTrackColumns(), ", "),
			") VALUES ",
			String::join(tuples.albumTuples, ", ")
		}), tuples.albumParams);
	}
	if(tuples.trackTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO Track (",
			String::join(trackTupleColumns(), ", "),
			") VALUES ",
			String::join(tuples.trackTuples, ", ")
		}), tuples.trackParams);
	}
	if(tuples.trackArtistTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO TrackArtist (",
			String::join(trackArtistTupleColumns(), ", "),
			") VALUES ",
			String::join(tuples.trackArtistTuples, ", ")
		}), tuples.trackArtistParams);
	}
	if(tuples.albumItemTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO TrackCollectionItem (",
			String::join(albumItemTupleFromTrackColumns(), ", "),
			") VALUES ",
			String::join(tuples.albumItemTuples, ", ")
		}), tuples.albumItemParams);
	}
}

void insertOrReplaceTracks(SQLiteTransaction& tx, const ArrayList<$<Track>>& tracks, bool includeAlbums) {
	if(tracks.size() == 0) {
		return;
	}
	TrackTuplesAndParams tuples;
	for(auto& track : tracks) {
		addTrackTuples(track, tuples, includeAlbums);
	}
	applyTrackTuples(tx, tuples);
}

struct TrackCollectionTuplesAndParams {
	LinkedList<String> collectionTuples;
	LinkedList<Any> collectionParams;
	LinkedList<String> fullUserAccountTuples;
	LinkedList<Any> fullUserAccountParams;
	LinkedList<String> partialUserAccountTuples;
	LinkedList<Any> partialUserAccountParams;
	LinkedList<String> fullArtistTuples;
	LinkedList<Any> fullArtistParams;
	LinkedList<String> partialArtistTuples;
	LinkedList<Any> partialArtistParams;
	LinkedList<String> collectionArtistTuples;
	LinkedList<Any> collectionArtistParams;
};

void addTrackCollectionTuples($<TrackCollection> collection, TrackCollectionTuplesAndParams& tuples) {
	tuples.collectionTuples.pushBack(trackCollectionTuple(tuples.collectionParams, collection, {.coalesce=true}));
	auto owner = trackCollectionOwner(collection);
	if(owner) {
		if(owner->needsData()) {
			tuples.partialUserAccountTuples.pushBack(userAccountTuple(tuples.partialUserAccountParams, owner, {.coalesce = true}));
		} else {
			tuples.fullUserAccountTuples.pushBack(userAccountTuple(tuples.fullUserAccountParams, owner, {.coalesce = false}));
		}
	}
	auto artists = trackCollectionArtists(collection);
	for(auto& artist : artists.valueOr(ArrayList<$<Artist>>())) {
		if(artist->uri().empty()) {
			continue;
		}
		if(artist->needsData()) {
			tuples.partialArtistTuples.pushBack(artistTuple(tuples.partialArtistParams, artist, {.coalesce=true}));
		} else {
			tuples.fullArtistTuples.pushBack(artistTuple(tuples.fullArtistParams, artist, {.coalesce=false}));
		}
		tuples.collectionArtistTuples.pushBack(trackCollectionArtistTuple(tuples.collectionArtistParams, {
			.collectionURI=collection->uri(),
			.artistURI=artist->uri()
		}));
	}
}

void applyTrackCollectionTuples(SQLiteTransaction& tx, TrackCollectionTuplesAndParams& tuples) {
	if(tuples.fullUserAccountTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO UserAccount (",
			String::join(userAccountTupleColumns(), ", "),
			") VALUES ",
			String::join(tuples.fullUserAccountTuples, ", ")
		}), tuples.fullUserAccountParams);
	}
	if(tuples.partialUserAccountTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO UserAccount (",
			String::join(userAccountTupleColumns(), ", "),
			") VALUES ",
			String::join(tuples.partialUserAccountTuples, ", ")
		}), tuples.partialUserAccountParams);
	}
	if(tuples.fullArtistTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO Artist (",
			String::join(artistTupleColumns(), ", "),
			") VALUES ",
			String::join(tuples.fullArtistTuples, ", ")
		}), tuples.fullArtistParams);
	}
	if(tuples.partialArtistTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO Artist (",
			String::join(artistTupleColumns(), ", "),
			") VALUES ",
			String::join(tuples.partialArtistTuples, ", ")
		}), tuples.partialArtistParams);
	}
	if(tuples.collectionTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO TrackCollection (",
			String::join(trackCollectionTupleColumns(), ", "),
			") VALUES ",
			String::join(tuples.collectionTuples, ", ")
		}), tuples.collectionParams);
	}
	if(tuples.collectionArtistTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO TrackCollectionArtist (",
			String::join(trackCollectionArtistTupleColumns(), ", "),
			") VALUES ",
			String::join(tuples.collectionArtistTuples, ", ")
		}), tuples.collectionArtistParams);
	}
}

void insertOrReplaceTrackCollections(SQLiteTransaction& tx, const ArrayList<$<TrackCollection>>& collections) {
	if(collections.size() == 0) {
		return;
	}
	TrackCollectionTuplesAndParams tuples;
	for(auto& collection : collections) {
		addTrackCollectionTuples(collection, tuples);
	}
	applyTrackCollectionTuples(tx, tuples);
}

void insertOrReplaceItemsFromTrackCollection(SQLiteTransaction& tx, $<TrackCollection> collection, InsertTrackCollectionItemsOptions options) {
	if(collection->itemCount()) {
		tx.addSQL(
			"DELETE FROM TrackCollectionItem WHERE collectionURI = ? AND indexNum >= ?",
			{ collection->uri(), collection->itemCount().toAny() });
	}
	TrackTuplesAndParams tuples;
	LinkedList<String> collectionItemTuples;
	LinkedList<Any> collectionItemParams;
	if(options.range) {
		collection->forEachInRange(options.range->startIndex, options.range->endIndex, [&]($<TrackCollectionItem> item, size_t index) {
			if(!item) {
				return;
			}
			collectionItemTuples.pushBack(trackCollectionItemTuple(collectionItemParams, item));
			addTrackTuples(item->track(), tuples, options.includeTrackAlbums);
		});
	}
	applyTrackTuples(tx, tuples);
	if(collectionItemTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO TrackCollectionItem (",
			String::join(trackCollectionItemTupleColumns(), ", "),
			") VALUES ",
			String::join(collectionItemTuples, ", ")
		}), collectionItemParams);
	}
}

void insertOrReplaceLibraryItems(SQLiteTransaction& tx, const ArrayList<MediaProvider::LibraryItem>& items) {
	TrackTuplesAndParams trackTuples;
	TrackCollectionTuplesAndParams collectionTuples;
	LinkedList<String> savedTrackTuples;
	LinkedList<Any> savedTrackParams;
	LinkedList<String> savedAlbumTuples;
	LinkedList<Any> savedAlbumParams;
	LinkedList<String> savedPlaylistTuples;
	LinkedList<Any> savedPlaylistParams;
	LinkedList<String> followedArtistTuples;
	LinkedList<Any> followedArtistParams;
	LinkedList<String> followedUserAccountTuples;
	LinkedList<Any> followedUserAccountParams;
	for(auto& item : items) {
		if(auto track = std::dynamic_pointer_cast<Track>(item.mediaItem)) {
			savedTrackTuples.pushBack(savedTrackTuple(savedTrackParams, {
				.trackURI = track->uri(),
				.libraryProvider = track->mediaProvider()->name(),
				.addedAt = item.addedAt.toISOString()
			}));
			addTrackTuples(track, trackTuples, true);
		}
		else if(auto album = std::dynamic_pointer_cast<Album>(item.mediaItem)) {
			savedAlbumTuples.pushBack(savedAlbumTuple(savedAlbumParams, {
				.albumURI = album->uri(),
				.libraryProvider = album->mediaProvider()->name(),
				.addedAt = item.addedAt.toISOString()
			}));
			addTrackCollectionTuples(album, collectionTuples);
		}
		else if(auto playlist = std::dynamic_pointer_cast<Playlist>(item.mediaItem)) {
			savedPlaylistTuples.pushBack(savedPlaylistTuple(savedPlaylistParams, {
				.playlistURI = playlist->uri(),
				.libraryProvider = playlist->mediaProvider()->name(),
				.addedAt = item.addedAt.toISOString()
			}));
			addTrackCollectionTuples(playlist, collectionTuples);
		}
		else if(auto artist = std::dynamic_pointer_cast<Artist>(item.mediaItem)) {
			followedArtistTuples.pushBack(followedArtistTuple(followedArtistParams, {
				.artistURI = artist->uri(),
				.libraryProvider = artist->mediaProvider()->name(),
				.addedAt = item.addedAt.toISOString()
			}));
			if(artist->needsData()) {
				trackTuples.partialArtistTuples.pushBack(artistTuple(trackTuples.partialArtistParams, artist, {.coalesce=true}));
			} else {
				trackTuples.fullArtistTuples.pushBack(artistTuple(trackTuples.fullArtistParams, artist, {.coalesce=false}));
			}
		}
		else if(auto userAccount = std::dynamic_pointer_cast<UserAccount>(item.mediaItem)) {
			followedUserAccountTuples.pushBack(followedUserAccountTuple(followedUserAccountParams, {
				.userURI = userAccount->uri(),
				.libraryProvider = userAccount->mediaProvider()->name(),
				.addedAt = item.addedAt.toISOString()
			}));
			if(artist->needsData()) {
				collectionTuples.partialUserAccountTuples.pushBack(userAccountTuple(collectionTuples.partialUserAccountParams, userAccount, {.coalesce=true}));
			} else {
				collectionTuples.fullUserAccountTuples.pushBack(userAccountTuple(collectionTuples.fullUserAccountParams, userAccount, {.coalesce=false}));
			}
		}
		else {
			throw std::runtime_error("Invalid MediaItem type for LibraryItem");
		}
	}
	applyTrackTuples(tx, trackTuples);
	applyTrackCollectionTuples(tx, collectionTuples);
	if(savedTrackTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO SavedTrack (",
			String::join(savedTrackTupleColumns(), ", "),
			") VALUES ",
			String::join(savedTrackTuples, ", ")
		}), savedTrackParams);
	}
	if(savedAlbumTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO SavedAlbum (",
			String::join(savedAlbumTupleColumns(), ", "),
			") VALUES ",
			String::join(savedAlbumTuples, ", ")
		}), savedAlbumParams);
	}
	if(savedPlaylistTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO SavedPlaylist (",
			String::join(savedPlaylistTupleColumns(), ", "),
			") VALUES ",
			String::join(savedPlaylistTuples, ", ")
		}), savedPlaylistParams);
	}
}

void insertOrReplaceSavedTracks(SQLiteTransaction& tx, const ArrayList<SavedTrack>& savedTracks) {
	LinkedList<String> savedTrackTuples;
	LinkedList<Any> savedTrackParams;
	for(auto& savedTrack : savedTracks) {
		savedTrackTuples.pushBack(savedTrackTuple(savedTrackParams, savedTrack));
	}
	if(savedTrackTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO SavedTrack (",
			String::join(savedTrackTupleColumns(), ", "),
			") VALUES ",
			String::join(savedTrackTuples, ", ")
		}), savedTrackParams);
	}
}

void insertOrReplaceSavedAlbums(SQLiteTransaction& tx, const ArrayList<SavedAlbum>& savedAlbums) {
	LinkedList<String> savedAlbumTuples;
	LinkedList<Any> savedAlbumParams;
	for(auto& savedAlbum : savedAlbums) {
		savedAlbumTuples.pushBack(savedAlbumTuple(savedAlbumParams, savedAlbum));
	}
	if(savedAlbumTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO SavedAlbum (",
			String::join(savedAlbumTupleColumns(), ", "),
			") VALUES ",
			String::join(savedAlbumTuples, ", ")
		}), savedAlbumParams);
	}
}

void insertOrReplaceSavedPlaylists(SQLiteTransaction& tx, const ArrayList<SavedPlaylist>& savedPlaylists) {
	LinkedList<String> savedPlaylistTuples;
	LinkedList<Any> savedPlaylistParams;
	for(auto& savedPlaylist : savedPlaylists) {
		savedPlaylistTuples.pushBack(savedPlaylistTuple(savedPlaylistParams, savedPlaylist));
	}
	if(savedPlaylistTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO SavedPlaylist (",
			String::join(savedPlaylistTupleColumns(), ", "),
			") VALUES ",
			String::join(savedPlaylistTuples, ", ")
		}), savedPlaylistParams);
	}
}

void insertOrReplacePlaybackHistoryItems(SQLiteTransaction& tx, const ArrayList<$<PlaybackHistoryItem>>& items) {
	TrackTuplesAndParams trackTuples;
	LinkedList<String> historyItemTuples;
	LinkedList<Any> historyItemParams;
	for(auto& item : items) {
		addTrackTuples(item->track(), trackTuples, true);
		historyItemTuples.pushBack(playbackHistoryItemTuple(historyItemParams, item));
	}
	applyTrackTuples(tx, trackTuples);
	if(historyItemTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO PlaybackHistoryItem (",
			String::join(playbackHistoryItemTupleColumns(), ", "),
			") VALUES ",
			String::join(historyItemTuples, ", ")
		}), historyItemParams);
	}
}

void insertOrReplaceDBStates(SQLiteTransaction& tx, const ArrayList<DBState>& states) {
	LinkedList<String> dbStateTuples;
	LinkedList<Any> dbStateParams;
	for(auto& dbState : states) {
		dbStateTuples.pushBack(dbStateTuple(dbStateParams, dbState));
	}
	if(dbStateTuples.size() > 0) {
		tx.addSQL(String::join({
			"INSERT OR REPLACE INTO DBState (",
			String::join(dbStateTupleColumns(), ", "),
			") VALUES ",
			String::join(dbStateTuples, ", ")
		}), dbStateParams);
	}
}

void applyDBState(SQLiteTransaction& tx, std::map<String,String> state) {
	if(state.size() == 0) {
		return;
	}
	ArrayList<sql::DBState> states;
	states.reserve(state.size());
	for(auto& pair : state) {
		states.pushBack({
			.stateKey=pair.first,
			.stateValue=pair.second
		});
	}
	insertOrReplaceDBStates(tx, states);
}

void selectTrack(SQLiteTransaction& tx, String outKey, String uri) {
	tx.addSQL("SELECT * FROM Track WHERE uri = ?", { uri }, {
		.outKey = outKey,
		.mapper = [](auto json) {
			return transformDBTrack(json);
		}
	});
}

void selectTrackCount(SQLiteTransaction& tx, String outKey) {
	tx.addSQL("SELECT count(*) AS total FROM Track", {}, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			return row["total"];
		}
	});
}

void selectTrackCollectionWithOwner(SQLiteTransaction& tx, String outKey, String uri) {
	auto joinTables = ArrayList<JoinTable>{
		{
			.name = "TrackCollection",
			.prefix = "r1_",
			.columns = trackCollectionColumns()
		}, {
			.name = "UserAccount",
			.prefix = "r2_",
			.columns = userAccountColumns()
		}
	};
	auto columns = joinedTableColumns(joinTables);
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT ",columns," FROM TrackCollection "
			"LEFT OUTER JOIN UserAccount ON TrackCollection.ownerURI = UserAccount.uri "
			"WHERE TrackCollection.uri = ",sqlParam(params, uri)
	});
	tx.addSQL(query, params, {
		.outKey = outKey,
		.mapper = [=](auto json) {
			auto results = splitJoinedResults(joinTables, json);
			return transformDBTrackCollection(results[0], results[1]);
		}
	});
}

void selectTrackCollectionItemsWithTracks(SQLiteTransaction& tx, String outKey, String collectionURI, Optional<IndexRange> range) {
	auto joinTables = ArrayList<JoinTable>{
		{
			.name = "TrackCollectionItem",
			.prefix = "r1_",
			.columns = trackCollectionItemColumns()
		}, {
			.name = "Track",
			.prefix = "r2_",
			.columns = trackColumns()
		}
	};
	auto columns = joinedTableColumns(joinTables);
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT ",columns," FROM TrackCollectionItem, Track WHERE TrackCollectionItem.collectionURI = ",sqlParam(params,collectionURI),
		" AND TrackCollectionItem.trackURI = Track.uri",
		(range ? (
			String::join({
				" AND TrackCollectionItem.indexNum >= ",sqlParam(params, range->startIndex),
				" AND TrackCollectionItem.indexNum < ",sqlParam(params, range->endIndex)
			})
		) : ""),
		" ORDER BY TrackCollectionItem.indexNum ASC"
	});
	tx.addSQL(query, params, {
		.outKey = outKey,
		.mapper = [=](auto row) {
			auto results = splitJoinedResults(joinTables, row);
			return transformDBTrackCollectionItem(results[0], results[1]);
		}
	});
}

void selectArtist(SQLiteTransaction& tx, String outKey, String uri) {
	tx.addSQL("SELECT * FROM Artist WHERE uri = ?", { uri }, {
		.outKey = outKey,
		.mapper = [](auto json) {
			return transformDBArtist(json);
		}
	});
}



void selectSavedTracksWithTracks(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options) {
	auto joinTables = ArrayList<JoinTable>{
		{
			.name = "SavedTrack",
			.prefix = "r1_",
			.columns = savedTrackColumns()
		}, {
			.name = "Track",
			.prefix = "r2_",
			.columns = trackColumns()
		}
	};
	auto columns = joinedTableColumns(joinTables);
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT ",columns," FROM SavedTrack, Track WHERE SavedTrack.trackURI = Track.uri",
		(options.libraryProvider.empty()) ?
			String()
			: String::join({
				" AND libraryProvider = ",sqlParam(params, options.libraryProvider),
			}),
		" ORDER BY ",([&]() {
			switch(options.orderBy) {
				case LibraryItemOrderBy::ADDED_AT:
					return "SavedTrack.addedAt";
				case LibraryItemOrderBy::NAME:
					return "TRIM(TRIM(TRIM(LOWER(Track.name),'\"'),\"'\"),'-')";
			}
			throw std::runtime_error("invalid LibraryItemOrderBy value");
		})(),([&]() {
			switch(options.order) {
				case Order::DEFAULT:
					return "";
				case Order::ASC:
					return " ASC";
				case Order::DESC:
					return " DESC";
			}
			return "";
		})(),
		sqlOffsetAndLimitFromRange(options.range, params)
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			auto results = splitJoinedResults(joinTables, row);
			return transformDBSavedTrack(results[0], results[1]);
		}
	});
}

void selectSavedTrackCount(SQLiteTransaction& tx, String outKey, String libraryProvider) {
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT count(*) AS total FROM SavedTrack",
		(libraryProvider.empty()) ?
			String()
			: String::join({
				" WHERE libraryProvider = ",sqlParam(params, libraryProvider),
			})
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			return row["total"];
		}
	});
}

void selectSavedTrack(SQLiteTransaction& tx, String outKey, String uri) {
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT * FROM SavedTrack WHERE SavedTrack.trackURI = ",sqlParam(params, uri)," LIMIT 1"
	});
	tx.addSQL(query, params, { .outKey=outKey });
}

void selectSavedTrackWithTrack(SQLiteTransaction& tx, String outKey, String uri) {
	auto joinTables = ArrayList<JoinTable>{
		{
			.name = "SavedTrack",
			.prefix = "r1_",
			.columns = savedTrackColumns()
		}, {
			.name = "Track",
			.prefix = "r2_",
			.columns = trackColumns()
		}
	};
	auto columns = joinedTableColumns(joinTables);
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT ",columns," FROM SavedTrack WHERE SavedTrack.trackURI = ",sqlParam(params, uri)," AND Track.uri = SavedTrack.trackURI LIMIT 1"
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			auto results = splitJoinedResults(joinTables, row);
			return transformDBSavedTrack(results[0], results[1]);
		}
	});
}



void selectSavedAlbumsWithAlbums(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options) {
	auto joinTables = ArrayList<JoinTable>{
		{
			.name = "SavedAlbum",
			.prefix = "r1_",
			.columns = savedAlbumColumns()
		}, {
			.name = "TrackCollection",
			.prefix = "r2_",
			.columns = trackCollectionColumns()
		}
	};
	auto columns = joinedTableColumns(joinTables);
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT ",columns," FROM SavedAlbum, TrackCollection WHERE SavedAlbum.albumURI = TrackCollection.uri",
		(options.libraryProvider.empty()) ?
			String()
			: String::join({
				" AND libraryProvider = ",sqlParam(params, options.libraryProvider),
			}),
		" ORDER BY ",([&]() {
			switch(options.orderBy) {
				case LibraryItemOrderBy::ADDED_AT:
					return "SavedAlbum.addedAt";
				case LibraryItemOrderBy::NAME:
					return "TRIM(TRIM(TRIM(LOWER(TrackCollection.name),'\"'),\"'\"),'-')";
			}
			throw std::runtime_error("invalid LibraryItemOrderBy value");
		})(),([&]() {
			switch(options.order) {
				case Order::DEFAULT:
					return "";
				case Order::ASC:
					return " ASC";
				case Order::DESC:
					return " DESC";
			}
			return "";
		})(),
		sqlOffsetAndLimitFromRange(options.range, params)
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			auto results = splitJoinedResults(joinTables, row);
			return transformDBSavedAlbum(results[0], results[1]);
		}
	});
}

void selectSavedAlbumCount(SQLiteTransaction& tx, String outKey, String libraryProvider) {
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT count(*) AS total FROM SavedAlbum",
		(libraryProvider.empty()) ?
			String()
			: String::join({
				" WHERE libraryProvider = ",sqlParam(params, libraryProvider),
			})
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			return row["total"];
		}
	});
}

void selectSavedAlbum(SQLiteTransaction& tx, String outKey, String uri) {
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT * FROM SavedAlbum WHERE SavedAlbum.albumURI = ",sqlParam(params, uri)," LIMIT 1"
	});
	tx.addSQL(query, params, { .outKey=outKey });
}

void selectSavedAlbumWithAlbum(SQLiteTransaction& tx, String outKey, String uri) {
	auto joinTables = ArrayList<JoinTable>{
		{
			.name = "SavedAlbum",
			.prefix = "r1_",
			.columns = savedAlbumColumns()
		}, {
			.name = "TrackCollection",
			.prefix = "r2_",
			.columns = trackCollectionColumns()
		}
	};
	auto columns = joinedTableColumns(joinTables);
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT ",columns," FROM SavedAlbum, TrackCollection WHERE SavedAlbum.albumURI = ",sqlParam(params, uri)," AND TrackCollection.uri = SavedAlbum.albumURI LIMIT 1"
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			auto results = splitJoinedResults(joinTables, row);
			return transformDBSavedAlbum(results[0], results[1]);
		}
	});
}



void selectSavedPlaylistsWithPlaylistsAndOwners(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options) {
	auto joinTables = ArrayList<JoinTable>{
		{
			.name = "SavedPlaylist",
			.prefix = "r1_",
			.columns = savedPlaylistColumns()
		}, {
			.name = "TrackCollection",
			.prefix = "r2_",
			.columns = trackCollectionColumns()
		}, {
			.name = "UserAccount",
			.prefix = "r3_",
			.columns = userAccountColumns()
		}
	};
	auto columns = joinedTableColumns(joinTables);
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT ",columns," FROM SavedPlaylist, "
			"TrackCollection LEFT OUTER JOIN UserAccount ON TrackCollection.ownerURI = UserAccount.uri "
		"WHERE SavedPlaylist.playlistURI = TrackCollection.uri",
		(options.libraryProvider.empty()) ?
			String()
			: String::join({
				" AND libraryProvider = ",sqlParam(params, options.libraryProvider),
			}),
		" ORDER BY ",([&]() {
			switch(options.orderBy) {
				case LibraryItemOrderBy::ADDED_AT:
					return "SavedPlaylist.addedAt";
				case LibraryItemOrderBy::NAME:
					return "TRIM(TRIM(TRIM(LOWER(TrackCollection.name),'\"'),\"'\"),'-')";
			}
			throw std::runtime_error("invalid LibraryItemOrderBy value");
		})(),([&]() {
			switch(options.order) {
				case Order::DEFAULT:
					return "";
				case Order::ASC:
					return " ASC";
				case Order::DESC:
					return " DESC";
			}
			return "";
		})(),
		sqlOffsetAndLimitFromRange(options.range, params)
	});
	tx.addSQL(query, params, {
		.outKey = outKey,
		.mapper = [=](auto row) {
			auto results = splitJoinedResults(joinTables, row);
			return transformDBSavedPlaylist(results[0], results[1], results[2]);
		}
	});
}

void selectSavedPlaylistCount(SQLiteTransaction& tx, String outKey, String libraryProvider) {
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT count(*) AS total FROM SavedPlaylist",
		(libraryProvider.empty()) ?
			String()
			: String::join({
				" WHERE libraryProvider = ",sqlParam(params, libraryProvider),
			})
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			return row["total"];
		}
	});
}

void selectSavedPlaylist(SQLiteTransaction& tx, String outKey, String uri) {
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT * FROM SavedPlaylist WHERE SavedPlaylist.playlistURI = ",sqlParam(params, uri)," LIMIT 1"
	});
	tx.addSQL(query, params, { .outKey=outKey });
}

void selectSavedPlaylistWithPlaylistAndOwner(SQLiteTransaction& tx, String outKey, String uri) {
	auto joinTables = ArrayList<JoinTable>{
		{
			.name = "SavedPlaylist",
			.prefix = "r1_",
			.columns = savedPlaylistColumns()
		}, {
			.name = "TrackCollection",
			.prefix = "r2_",
			.columns = trackCollectionColumns()
		}, {
			.name = "UserAccount",
			.prefix = "r3_",
			.columns = userAccountColumns()
		}
	};
	auto columns = joinedTableColumns(joinTables);
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT ",columns," FROM SavedPlaylist, "
			"TrackCollection LEFT OUTER JOIN UserAccount ON TrackCollection.ownerURI = UserAccount.uri "
		"WHERE SavedPlaylist.playlistURI = ",sqlParam(params, uri)," AND TrackCollection.uri = SavedPlaylist.playlistURI LIMIT 1"
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			auto results = splitJoinedResults(joinTables, row);
			return transformDBSavedPlaylist(results[0], results[1], results[2]);
		}
	});
}



void selectFollowedArtistsWithArtists(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options) {
	auto joinTables = ArrayList<JoinTable>{
		{
			.name = "FollowedArtist",
			.prefix = "r1_",
			.columns = followedArtistColumns()
		}, {
			.name = "Artist",
			.prefix = "r2_",
			.columns = artistColumns()
		}
	};
	auto columns = joinedTableColumns(joinTables);
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT ",columns," FROM FollowedArtist, Artist WHERE FollowedArtist.artistURI = Artist.uri",
		(options.libraryProvider.empty()) ?
			String()
			: String::join({
				" AND libraryProvider = ",sqlParam(params, options.libraryProvider),
			}),
		" ORDER BY ",([&]() {
			switch(options.orderBy) {
				case LibraryItemOrderBy::ADDED_AT:
					return "FollowedArtist.addedAt";
				case LibraryItemOrderBy::NAME:
					return "TRIM(TRIM(TRIM(LOWER(Artist.name),'\"'),\"'\"),'-')";
			}
			throw std::runtime_error("invalid LibraryItemOrderBy value");
		})(),([&]() {
			switch(options.order) {
				case Order::DEFAULT:
					return "";
				case Order::ASC:
					return " ASC";
				case Order::DESC:
					return " DESC";
			}
			return "";
		})(),
		sqlOffsetAndLimitFromRange(options.range, params)
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			auto results = splitJoinedResults(joinTables, row);
			return transformDBFollowedArtist(results[0], results[1]);
		}
	});
}

void selectFollowedArtistCount(SQLiteTransaction& tx, String outKey, String libraryProvider) {
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT count(*) AS total FROM FollowedArtist",
		(libraryProvider.empty()) ?
			String()
			: String::join({
				" WHERE libraryProvider = ",sqlParam(params, libraryProvider),
			})
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			return row["total"];
		}
	});
}

void selectFollowedArtist(SQLiteTransaction& tx, String outKey, String uri) {
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT * FROM FollowedArtist WHERE FollowedArtist.artistURI = ",sqlParam(params, uri)," LIMIT 1"
	});
	tx.addSQL(query, params, { .outKey=outKey });
}

void selectFollowedArtistWithArtist(SQLiteTransaction& tx, String outKey, String uri) {
	auto joinTables = ArrayList<JoinTable>{
		{
			.name = "FollowedArtist",
			.prefix = "r1_",
			.columns = followedArtistColumns()
		}, {
			.name = "Artist",
			.prefix = "r2_",
			.columns = artistColumns()
		}
	};
	auto columns = joinedTableColumns(joinTables);
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT ",columns," FROM FollowedArtist, Artist WHERE FollowedArtist.artistURI = ",sqlParam(params, uri)," AND Artist.uri = FollowedArtist.artistURI LIMIT 1"
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			auto results = splitJoinedResults(joinTables, row);
			return transformDBFollowedArtist(results[0], results[1]);
		}
	});
}



void selectFollowedUserAccountsWithUserAccounts(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options) {
	auto joinTables = ArrayList<JoinTable>{
		{
			.name = "FollowedUserAccount",
			.prefix = "r1_",
			.columns = followedUserAccountColumns()
		}, {
			.name = "UserAccount",
			.prefix = "r2_",
			.columns = userAccountColumns()
		}
	};
	auto columns = joinedTableColumns(joinTables);
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT ",columns," FROM FollowedUserAccount, UserAccount WHERE FollowedUserAccount.userURI = UserAccount.uri",
		(options.libraryProvider.empty()) ?
			String()
			: String::join({
				" AND libraryProvider = ",sqlParam(params, options.libraryProvider),
			}),
		" ORDER BY ",([&]() {
			switch(options.orderBy) {
				case LibraryItemOrderBy::ADDED_AT:
					return "FollowedUserAccount.addedAt";
				case LibraryItemOrderBy::NAME:
					return "TRIM(TRIM(TRIM(LOWER(UserAccount.name),'\"'),\"'\"),'-')";
			}
			throw std::runtime_error("invalid LibraryItemOrderBy value");
		})(),([&]() {
			switch(options.order) {
				case Order::DEFAULT:
					return "";
				case Order::ASC:
					return " ASC";
				case Order::DESC:
					return " DESC";
			}
			return "";
		})(),
		sqlOffsetAndLimitFromRange(options.range, params)
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			auto results = splitJoinedResults(joinTables, row);
			return transformDBFollowedUserAccount(results[0], results[1]);
		}
	});
}

void selectFollowedUserAccountCount(SQLiteTransaction& tx, String outKey, String libraryProvider) {
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT count(*) AS total FROM FollowedUserAccount",
		(libraryProvider.empty()) ?
			String()
			: String::join({
				" WHERE libraryProvider = ",sqlParam(params, libraryProvider),
			})
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			return row["total"];
		}
	});
}

void selectFollowedUserAccount(SQLiteTransaction& tx, String outKey, String uri) {
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT * FROM FollowedUserAccount WHERE FollowedUserAccount.userURI = ",sqlParam(params, uri)," LIMIT 1"
	});
	tx.addSQL(query, params, { .outKey=outKey });
}

void selectFollowedUserAccountWithUserAccount(SQLiteTransaction& tx, String outKey, String uri) {
	auto joinTables = ArrayList<JoinTable>{
		{
			.name = "FollowedUserAccount",
			.prefix = "r1_",
			.columns = followedUserAccountColumns()
		}, {
			.name = "UserAccount",
			.prefix = "r2_",
			.columns = userAccountColumns()
		}
	};
	auto columns = joinedTableColumns(joinTables);
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT ",columns," FROM FollowedUserAccount, Artist WHERE FollowedUserAccount.userURI = ",sqlParam(params, uri)," AND UserAccount.uri = FollowedUserAccount.userURI LIMIT 1"
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			auto results = splitJoinedResults(joinTables, row);
			return transformDBFollowedUserAccount(results[0], results[1]);
		}
	});
}



void selectLibraryArtists(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options) {
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT * FROM Artist AS a "
		"WHERE ",
		(options.libraryProvider.empty()) ?
			String()
			: String::join({
				"libraryProvider = ",sqlParam(params, options.libraryProvider)," AND "
			}),
		"("
			"EXISTS("
				// if artist is attached to a saved track
				"SELECT TrackArtist.trackURI, TrackArtist.artistURI FROM TrackArtist, SavedTrack "
					"WHERE TrackArtist.artistURI = a.uri AND TrackArtist.trackURI = SavedTrack.trackURI"
			") "
			"OR EXISTS("
				// if artist is attached to a saved album
				"SELECT TrackCollectionArtist.trackURI, TrackCollectionArtist.artistURI FROM TrackCollectionArtist, SavedAlbum "
					"WHERE TrackCollectionArtist.artistURI = a.uri AND TrackCollectionArtist.collectionURI = SavedAlbum.albumURI"
			") "
			"OR EXISTS("
				// if artist is attached to a saved playlist
				"SELECT TrackCollectionArtist.trackURI, TrackCollectionArtist.artistURI FROM TrackCollectionArtist, SavedPlaylist "
					"WHERE TrackCollectionArtist.artistURI = a.uri AND TrackCollectionArtist.collectionURI = SavedPlaylist.playlistURI"
			") "
			"OR EXISTS("
				// if artist is followed
				"SELECT FollowedArtist.artistURI FROM FollowedArtist "
					"WHERE FollowedArtist.artistURI = a.uri"
			") "
		") "
		"ORDER BY ",([&]() {
			switch(options.orderBy) {
				case LibraryItemOrderBy::ADDED_AT:
					return "Artist.name";
				case LibraryItemOrderBy::NAME:
					return "TRIM(TRIM(TRIM(LOWER(Artist.name),'\"'),\"'\"),'-')";
			}
			throw std::runtime_error("invalid LibraryItemOrderBy value");
		})(),([&]() {
			switch(options.order) {
				case Order::DEFAULT:
					return "";
				case Order::ASC:
					return " ASC";
				case Order::DESC:
					return " DESC";
			}
			return "";
		})(),
		sqlOffsetAndLimitFromRange(options.range, params)
	});
	tx.addSQL(query, params, {
		.outKey = outKey,
		.mapper = [](auto json) {
			return transformDBArtist(json);
		}
	});
}

void selectLibraryArtistCount(SQLiteTransaction& tx, String outKey, String libraryProvider) {
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT count(*) AS total FROM Artist AS a "
		"WHERE ",
		(libraryProvider.empty()) ?
			String()
			: String::join({
				"libraryProvider = ",sqlParam(params, libraryProvider)," AND "
			}),
		"("
			"EXISTS("
				// if artist is attached to saved track
				"SELECT TrackArtist.trackURI, TrackArtist.artistURI FROM TrackArtist, SavedTrack "
					"WHERE TrackArtist.artistURI = a.uri AND TrackArtist.trackURI = SavedTrack.trackURI"
			") "
			"OR EXISTS("
				// if artist is attached to saved album
				"SELECT TrackCollectionArtist.trackURI, TrackCollectionArtist.artistURI FROM TrackCollectionArtist, SavedAlbum "
					"WHERE TrackCollectionArtist.artistURI = a.uri AND TrackCollectionArtist.collectionURI = SavedAlbum.albumURI"
			") "
			"OR EXISTS("
				// if artist is attached to saved playlist
				"SELECT TrackCollectionArtist.trackURI, TrackCollectionArtist.artistURI FROM TrackCollectionArtist, SavedPlaylist "
					"WHERE TrackCollectionArtist.artistURI = a.uri AND TrackCollectionArtist.collectionURI = SavedPlaylist.playlistURI"
			") "
		")"
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			return row["total"];
		}
	});
}



String PlaybackHistorySelectFilters::sql(LinkedList<Any>& params) const {
	return String::join(ArrayList<String>{
		// trackURIs
		(!trackURIs.empty()) ?
			String::join({
				"(",
				String::join(trackURIs.map([&](auto& trackURI) {
					return String::join({ "PlaybackHistoryItem.trackURI = ",sqlParam(params, trackURI) });
				}), " OR "),
				")"
			})
			: String(),
		// minDate
		(minDate.hasValue()) ?
			String::join({ "PlaybackHistoryItem.startTime >= ",sqlParam(params, minDate->toISOString()) })
			: String(),
		// maxDate
		(maxDate.hasValue()) ?
			String::join({ "PlaybackHistoryItem.startTime < ",sqlParam(params, maxDate->toISOString()) })
			: String(),
		// includeNullDuration
		(!includeNullDuration.valueOr(true)) ?
			"PlaybackHistoryItem.duration IS NOT NULL"
			: String(),
		// minDuration or minDurationRatio
		(minDuration.hasValue() || minDurationRatio.hasValue()) ?
			String::join({
				// minDuration
				minDuration.hasValue() ?
					String::join({
						"(",
						includeNullDuration.valueOr(true) ?
							"PlaybackHistoryItem.duration IS NULL OR "
							: String(),
						"PlaybackHistoryItem.duration >= ",sqlParam(params, minDuration.value()),
						")"
					})
					: String(),
				// minDurationRatio
				minDurationRatio.hasValue() ?
					String::join({
						minDuration.hasValue() ? " AND " : String(),
						"(",
						includeNullDuration.valueOr(true) ?
							"PlaybackHistoryItem.duration IS NULL OR "
							: String(),
						"PlaybackHistoryItem.duration >= "
							"("
							"COALESCE(Track.duration, ",sqlParam(params, PlaybackHistoryItem::FALLBACK_DURATION),")"
							" * ",sqlParam(params, minDurationRatio.hasValue()),
							")"
						")"
				   })
				   : String(),
			})
			: String()
	}.where([](auto& str) {return !str.empty();}), " AND ");
}

void selectPlaybackHistoryItemsWithTracks(SQLiteTransaction& tx, String outKey, const PlaybackHistorySelectFilters& filters, const PlaybackHistorySelectOptions& options) {
	auto joinTables = ArrayList<JoinTable>{
		{
			.name = "PlaybackHistoryItem",
			.prefix = "r1_",
			.columns = playbackHistoryItemColumns()
		}, {
			.name = "Track",
			.prefix = "r2_",
			.columns = trackColumns()
		}
	};
	auto columns = joinedTableColumns(joinTables);
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT ",columns," FROM PlaybackHistoryItem, Track WHERE PlaybackHistoryItem.trackURI = Track.uri",
		([&]() {
			auto sql = filters.sql(params);
			if(sql.empty()) {
				return String();
			}
			return " AND " + sql;
		})(),
		" ORDER BY PlaybackHistoryItem.startTime",([&]() -> String {
			switch(options.order) {
				case Order::DEFAULT:
					return "";
				case Order::ASC:
					return " ASC";
				case Order::DESC:
					return " DESC";
			}
			throw std::invalid_argument("invalid value for options.order");
		})(),
		sqlOffsetAndLimitFromRange(options.range, params)
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			auto results = splitJoinedResults(joinTables, row);
			return transformDBPlaybackHistoryItem(results[0], results[1]);
		}
	});
}

void selectPlaybackHistoryItemCount(SQLiteTransaction& tx, String outKey, const PlaybackHistorySelectFilters& filters) {
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT count(*) AS total FROM FollowedUserAccount",
		([&]() {
			auto sql = filters.sql(params);
			if(sql.empty()) {
				return String();
			}
			return " WHERE "+sql;
		})()
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			return row["total"];
		}
	});
}



void selectDBState(SQLiteTransaction& tx, String outKey, String stateKey) {
	tx.addSQL("SELECT * FROM DBState WHERE stateKey = ?", { stateKey }, { .outKey=outKey });
}




void updateTrackCollectionVersionId(SQLiteTransaction& tx, String collectionURI, String versionId) {
	LinkedList<Any> params;
	tx.addSQL("UPDATE TrackCollection SET versionId = ? WHERE uri = ?", { versionId, collectionURI });
}



void deleteSavedTrack(SQLiteTransaction& tx, String trackURI) {
	tx.addSQL("DELETE FROM SavedTrack AS tr WHERE trackURI = ?", {
		trackURI
	});
}

void deleteSavedAlbum(SQLiteTransaction& tx, String albumURI) {
	tx.addSQL("DELETE FROM SavedAlbum AS tr WHERE albumURI = ?", { albumURI });
}

void deleteSavedPlaylist(SQLiteTransaction& tx, String playlistURI) {
	tx.addSQL("DELETE FROM SavedPlaylist AS tr WHERE playlistURI = ?", { playlistURI });
}

void deleteFollowedArtist(SQLiteTransaction& tx, String artistURI) {
	tx.addSQL("DELETE FROM FollowedArtist AS tr WHERE artistURI = ?", { artistURI });
}

void deleteFollowedUserAccount(SQLiteTransaction& tx, String userURI) {
	tx.addSQL("DELETE FROM FollowedUserAccount AS tr WHERE userURI = ?", { userURI });
}

void deleteNonLibraryCollectionItems(SQLiteTransaction& tx) {
	tx.addSQL(
		"DELETE FROM TrackCollectionItem AS tci "
		"WHERE NOT EXISTS("
			// check if collection is a saved album
			"SELECT albumURI FROM SavedAlbum WHERE SavedAlbum.albumURI = tci.collectionURI"
		") "
		"AND NOT EXISTS("
			// check if collection is a saved playlist
			"SELECT playlistURI FROM SavedPlaylist WHERE SavedPlaylist.playlistURI = tci.collectionURI"
		")", {});
}

void deleteNonLibraryTracks(SQLiteTransaction& tx) {
	// delete from track artists
	tx.addSQL(
		"DELETE FROM TrackArtist AS ta "
		"WHERE NOT EXISTS("
			// check if track is a saved track
			"SELECT trackURI, libraryProvider FROM SavedTrack WHERE SavedTrack.trackURI = ta.trackURI"
		") "
		"AND NOT EXISTS("
			// check if a collection item from a saved album references this track
			"SELECT TrackCollectionItem.collectionURI, TrackCollectionItem.indexNum, TrackCollectionItem.trackURI "
				"FROM TrackCollectionItem, SavedAlbum "
				"WHERE TrackCollectionItem.trackURI = ta.trackURI AND TrackCollectionItem.collectionURI = SavedAlbum.albumURI"
		") "
		"AND NOT EXISTS("
			// check if a collection item from a saved playlist references this track
			"SELECT TrackCollectionItem.collectionURI, TrackCollectionItem.indexNum, TrackCollectionItem.trackURI "
				"FROM TrackCollectionItem, SavedPlaylist "
				"WHERE TrackCollectionItem.trackURI = ta.trackURI AND TrackCollectionItem.collectionURI = SavedPlaylist.playlistURI"
		") "
		"AND NOT EXISTS("
			// check if track is from a saved album
			"SELECT Track.uri FROM Track, SavedAlbum WHERE Track.uri = ta.trackURI AND Track.albumURI = SavedAlbum.albumURI"
		")", {});
	// delete from tracks
	tx.addSQL(
		"DELETE FROM Track AS t "
		"WHERE NOT EXISTS("
			// check if track is a saved track
			"SELECT trackURI, libraryProvider FROM SavedTrack WHERE SavedTrack.trackURI = t.uri"
		") "
		"AND NOT EXISTS("
			// check if a collection item from a saved album references this track
			"SELECT TrackCollectionItem.collectionURI, TrackCollectionItem.indexNum, TrackCollectionItem.trackURI "
				"FROM TrackCollectionItem, SavedAlbum "
				"WHERE TrackCollectionItem.trackURI = t.uri AND TrackCollectionItem.collectionURI = SavedAlbum.albumURI"
		") "
		"AND NOT EXISTS("
			// check if a collection item from a saved playlist references this track
			"SELECT TrackCollectionItem.collectionURI, TrackCollectionItem.indexNum, TrackCollectionItem.trackURI "
				"FROM TrackCollectionItem, SavedPlaylist "
				"WHERE TrackCollectionItem.trackURI = t.uri AND TrackCollectionItem.collectionURI = SavedPlaylist.playlistURI"
		") "
		"AND NOT EXISTS("
			// check if track is from a saved album
			"SELECT SavedAlbum.uri FROM SavedAlbum WHERE t.albumURI = SavedAlbum.albumURI"
		")", {});
}

void deleteNonLibraryCollections(SQLiteTransaction& tx) {
	// delete from collection artists
	tx.addSQL(
		"DELETE FROM TrackCollectionArtist AS tca "
		"WHERE NOT EXISTS("
			// check if collection is a saved album
			"SELECT albumURI, libraryProvider FROM SavedAlbum WHERE SavedAlbum.albumURI = tca.collectionURI"
		") "
		"WHERE NOT EXISTS("
			// check if collection is a saved playlist
			"SELECT playlistURI, libraryProvider FROM SavedPlaylist WHERE SavedPlaylist.playlistURI = tca.collectionURI"
		") "
		"AND NOT EXISTS("
			// check if collection is the album of a saved track
			"SELECT Track.albumURI FROM Track WHERE Track.albumURI = tca.collectionURI"
		")", {});
	tx.addSQL(
		"DELETE FROM TrackCollection AS tc "
		"WHERE NOT EXISTS("
			// check if collection is a saved album
			"SELECT albumURI, libraryProvider FROM SavedAlbum WHERE SavedAlbum.albumURI = tc.uri"
		") "
		"WHERE NOT EXISTS("
			// check if collection is a saved playlist
			"SELECT playlistURI, libraryProvider FROM SavedPlaylist WHERE SavedPlaylist.playlistURI = tc.uri"
		") "
		"AND NOT EXISTS("
			// check if collection is the album of a saved track
			"SELECT Track.albumURI FROM Track WHERE Track.albumURI = tc.uri"
		")", {});
}

void deleteNonLibraryArtists(SQLiteTransaction& tx) {
	tx.addSQL(
		"DELETE FROM Artist AS a "
		"WHERE NOT EXISTS("
			// check if artist is referenced by a track artist
			"SELECT artistURI FROM TrackArtist WHERE TrackArtist.artistURI = a.uri"
		") "
		"AND NOT EXISTS("
			// check if artist is referenced by a collection artist
			"SELECT artistURI FROM TrackCollectionArtist WHERE TrackCollectionArtist.artistURI = a.uri"
		") "
		"AND NOT EXISTS("
			// check if artist is a followed artist
			"SELECT artistURI FROM FollowedArtist WHERE FollowedArtist.artistURI = a.uri"
		") ", {});
}

void deleteNonLibraryUserAccounts(SQLiteTransaction& tx) {
	tx.addSQL(
		"DELETE FROM UserAccount AS u "
		"WHERE NOT EXISTS("
			  // check if user is referenced by a track collection
			  "SELECT ownerURI FROM TrackCollection WHERE TrackCollection.ownerURI = u.uri"
		") "
		"AND NOT EXISTS("
			  // check if user is a followed user
			  "SELECT userURI FROM FollowedUserAccount WHERE FollowedUserAccount.userURI = u.uri"
		") ", {});
}

}

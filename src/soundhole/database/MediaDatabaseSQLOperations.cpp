//
//  MediaDatabaseSQLOperations.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 5/25/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "MediaDatabaseSQLOperations.hpp"

namespace sh::sql {

struct JoinTable {
	String name;
	String prefix;
	ArrayList<String> columns;
};

String joinedTableColumns(ArrayList<JoinTable> tables) {
	return String::join(tables.reduce(LinkedList<String>{}, [](auto list, auto& table, auto index, auto self) {
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
	LinkedList<String> fullArtistTuples;
	LinkedList<Any> fullArtistParams;
	LinkedList<String> partialArtistTuples;
	LinkedList<Any> partialArtistParams;
	LinkedList<String> collectionArtistTuples;
	LinkedList<Any> collectionArtistParams;
};

void addTrackCollectionTuples($<TrackCollection> collection, TrackCollectionTuplesAndParams& tuples) {
	tuples.collectionTuples.pushBack(trackCollectionTuple(tuples.collectionParams, collection, {.coalesce=true}));
	auto artists = trackCollectionArtists(collection);
	for(auto& artist : artists) {
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
	for(auto& item : items) {
		if(auto track = std::dynamic_pointer_cast<Track>(item.mediaItem)) {
			savedTrackTuples.pushBack(savedTrackTuple(savedTrackParams, {
				.track=track,
				.libraryProvider=item.libraryProvider,
				.addedAt=item.addedAt
			}));
			addTrackTuples(track, trackTuples, true);
		}
		else if(auto album = std::dynamic_pointer_cast<Album>(item.mediaItem)) {
			savedAlbumTuples.pushBack(savedAlbumTuple(savedAlbumParams, {
				.album=album,
				.libraryProvider=item.libraryProvider,
				.addedAt=item.addedAt
			}));
			addTrackCollectionTuples(album, collectionTuples);
		}
		else if(auto playlist = std::dynamic_pointer_cast<Playlist>(item.mediaItem)) {
			savedPlaylistTuples.pushBack(savedPlaylistTuple(savedPlaylistParams, {
				.playlist=playlist,
				.libraryProvider=item.libraryProvider,
				.addedAt=item.addedAt
			}));
			addTrackCollectionTuples(playlist, collectionTuples);
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

void selectTrack(SQLiteTransaction& tx, String outKey, String uri) {
	tx.addSQL("SELECT * FROM Track WHERE uri = ?", { uri }, { .outKey=outKey });
}

void selectTrackCount(SQLiteTransaction& tx, String outKey) {
	tx.addSQL("SELECT count(*) AS total FROM Track", {}, { .outKey=outKey });
}

void selectTrackCollection(SQLiteTransaction& tx, String outKey, String uri) {
	tx.addSQL("SELECT * FROM TrackCollection WHERE uri = ?", { uri }, { .outKey=outKey });
}

void selectTrackCollectionItemsAndTracks(SQLiteTransaction& tx, String outKey, String collectionURI, Optional<IndexRange> range) {
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
		" ORDER BY indexNum ASC"
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			return Json(splitJoinedResults(joinTables, row));
		}
	});
}

void selectArtist(SQLiteTransaction& tx, String outKey, String uri) {
	tx.addSQL("SELECT * FROM Artist WHERE uri = ?", { uri }, { .outKey=outKey });
}

void selectSavedTracksAndTracks(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options) {
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
		" ORDER BY addedAt",([&]() {
			switch(options.order) {
				case Order::NONE:
					return "";
				case Order::ASC:
					return " ASC";
				case Order::DESC:
					return " DESC";
			}
			return "";
		})(),
		(options.range ?
			String::join({
				" LIMIT ",sqlParam(params, (options.range->endIndex - options.range->startIndex))," OFFSET ",sqlParam(params, options.range->startIndex)
			})
			: "")
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			return Json(splitJoinedResults(joinTables, row));
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
	tx.addSQL(query, params, { .outKey=outKey });
}

void selectSavedAlbumsAndAlbums(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options) {
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
		" ORDER BY addedAt",([&]() {
			switch(options.order) {
				case Order::NONE:
					return "";
				case Order::ASC:
					return " ASC";
				case Order::DESC:
					return " DESC";
			}
			return "";
		})(),
		(options.range ?
			String::join({
				" LIMIT ",sqlParam(params, (options.range->endIndex - options.range->startIndex))," OFFSET ",sqlParam(params, options.range->startIndex)
			})
			: "")
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			return Json(splitJoinedResults(joinTables, row));
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
	tx.addSQL(query, params, { .outKey=outKey });
}

void selectSavedPlaylistsAndPlaylists(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options) {
	auto joinTables = ArrayList<JoinTable>{
		{
			.name = "SavedPlaylist",
			.prefix = "r1_",
			.columns = savedTrackColumns()
		}, {
			.name = "TrackCollection",
			.prefix = "r2_",
			.columns = trackCollectionColumns()
		}
	};
	auto columns = joinedTableColumns(joinTables);
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT ",columns," FROM SavedPlaylist, TrackCollection WHERE SavedPlaylist.trackURI = TrackCollection.uri",
		(options.libraryProvider.empty()) ?
			String()
			: String::join({
				" AND libraryProvider = ",sqlParam(params, options.libraryProvider),
			}),
		" ORDER BY addedAt",([&]() {
			switch(options.order) {
				case Order::NONE:
					return "";
				case Order::ASC:
					return " ASC";
				case Order::DESC:
					return " DESC";
			}
			return "";
		})(),
		(options.range ?
			String::join({
				" LIMIT ",sqlParam(params, (options.range->endIndex - options.range->startIndex))," OFFSET ",sqlParam(params, options.range->startIndex)
			})
			: "")
	});
	tx.addSQL(query, params, {
		.outKey=outKey,
		.mapper=[=](auto row) {
			return Json(splitJoinedResults(joinTables, row));
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
	tx.addSQL(query, params, { .outKey=outKey });
}

void selectLibraryArtists(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options) {
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT Artist.* FROM Artist, TrackArtist, SavedTrack WHERE SavedTrack.trackURI = TrackArtist.trackURI AND TrackArtist.artistURI = Artist.uri",
		(options.libraryProvider.empty()) ?
			String()
			: String::join({
				" AND libraryProvider = ",sqlParam(params, options.libraryProvider),
			}),
		" GROUP BY Artist.uri ORDER BY Artist.name ASC",
		(options.range ?
			String::join({
				" LIMIT ",sqlParam(params, (options.range->endIndex - options.range->startIndex))," OFFSET ",sqlParam(params, options.range->startIndex)
			})
			: "")
	});
	tx.addSQL(query, params, { .outKey=outKey });
}

void selectLibraryArtistCount(SQLiteTransaction& tx, String outKey, String libraryProvider) {
	LinkedList<Any> params;
	auto query = String::join({
		"SELECT count(*) AS total FROM Artist, TrackArtist, SavedTrack WHERE SavedTrack.trackURI = TrackArtist.trackURI AND TrackArtist.artistURI = Artist.uri",
		(libraryProvider.empty()) ?
			String()
			: String::join({
				" AND libraryProvider = ",sqlParam(params, libraryProvider),
			}),
		" GROUP BY Artist.uri",
	});
	tx.addSQL(query, params, { .outKey=outKey });
}

void selectDBState(SQLiteTransaction& tx, String outKey, String stateKey) {
	tx.addSQL("SELECT * FROM DBState WHERE stateKey = ?", { stateKey }, { .outKey=outKey });
}

}

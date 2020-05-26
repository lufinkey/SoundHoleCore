//
//  MediaDatabaseSQLOperations.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 5/25/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "MediaDatabaseSQLOperations.hpp"

namespace sh::sql {

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
		tx.addSQL(String::join(ArrayList<String>{
			"INSERT OR REPLACE INTO Artist (",
			String::join(artistTupleColumns(), ", "),
			") VALUES ",
			String::join(fullArtistTuples, ", ")
		}), fullArtistParams);
	}
	if(partialArtistTuples.size() > 0) {
		tx.addSQL(String::join(ArrayList<String>{
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
		tx.addSQL(String::join(ArrayList<String>{
			"INSERT OR REPLACE INTO Artist (",
			String::join(artistTupleColumns(), ", "),
			") VALUES ",
			String::join(tuples.fullArtistTuples, ", ")
		}), tuples.fullArtistParams);
	}
	if(tuples.partialArtistTuples.size() > 0) {
		tx.addSQL(String::join(ArrayList<String>{
			"INSERT OR REPLACE INTO Artist (",
			String::join(artistTupleColumns(), ", "),
			") VALUES ",
			String::join(tuples.partialArtistTuples, ", ")
		}), tuples.partialArtistParams);
	}
	if(tuples.albumTuples.size() > 0) {
		tx.addSQL(String::join(ArrayList<String>{
			"INSERT OR REPLACE INTO TrackCollection (",
			String::join(albumTupleFromTrackColumns(), ", "),
			") VALUES ",
			String::join(tuples.albumTuples, ", ")
		}), tuples.albumParams);
	}
	if(tuples.trackTuples.size() > 0) {
		tx.addSQL(String::join(ArrayList<String>{
			"INSERT OR REPLACE INTO Track (",
			String::join(trackTupleColumns(), ", "),
			") VALUES ",
			String::join(tuples.trackTuples, ", ")
		}), tuples.trackParams);
	}
	if(tuples.trackArtistTuples.size() > 0) {
		tx.addSQL(String::join(ArrayList<String>{
			"INSERT OR REPLACE INTO TrackArtist (",
			String::join(trackArtistTupleColumns(), ", "),
			") VALUES ",
			String::join(tuples.trackArtistTuples, ", ")
		}), tuples.trackArtistParams);
	}
	if(tuples.albumItemTuples.size() > 0) {
		tx.addSQL(String::join(ArrayList<String>{
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
		tx.addSQL(String::join(ArrayList<String>{
			"INSERT OR REPLACE INTO Artist (",
			String::join(artistTupleColumns(), ", "),
			") VALUES ",
			String::join(tuples.fullArtistTuples, ", ")
		}), tuples.fullArtistParams);
	}
	if(tuples.partialArtistTuples.size() > 0) {
		tx.addSQL(String::join(ArrayList<String>{
			"INSERT OR REPLACE INTO Artist (",
			String::join(artistTupleColumns(), ", "),
			") VALUES ",
			String::join(tuples.partialArtistTuples, ", ")
		}), tuples.partialArtistParams);
	}
	if(tuples.collectionTuples.size() > 0) {
		tx.addSQL(String::join(ArrayList<String>{
			"INSERT OR REPLACE INTO TrackCollection (",
			String::join(trackCollectionTupleColumns(), ", "),
			") VALUES ",
			String::join(tuples.collectionTuples, ", ")
		}), tuples.collectionParams);
	}
	if(tuples.collectionArtistTuples.size() > 0) {
		tx.addSQL(String::join(ArrayList<String>{
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

void insertOrReplaceItemsFromTrackCollection(SQLiteTransaction& tx, $<TrackCollection> collection, Optional<IndexRange> range, bool includeTrackAlbums) {
	if(collection->itemCount()) {
		tx.addSQL(
			"DELETE FROM TrackCollectionItem WHERE collectionURI = ? AND indexNum >= ?",
			{ collection->uri(), collection->itemCount().toAny() });
	}
	TrackTuplesAndParams tuples;
	LinkedList<String> collectionItemTuples;
	LinkedList<Any> collectionItemParams;
	if(range) {
		collection->forEachInRange(range->startIndex, range->endIndex, [&]($<TrackCollectionItem> item, size_t index) {
			if(!item) {
				return;
			}
			collectionItemTuples.pushBack(trackCollectionItemTuple(collectionItemParams, item));
			addTrackTuples(item->track(), tuples, includeTrackAlbums);
		});
	}
	applyTrackTuples(tx, tuples);
	if(collectionItemTuples.size() > 0) {
		tx.addSQL(String::join(ArrayList<String>{
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
			addTrackCollectionTuples(album, collectionTuples);
		}
		else {
			throw std::runtime_error("Invalid MediaItem type for LibraryItem");
		}
	}
	applyTrackTuples(tx, trackTuples);
	applyTrackCollectionTuples(tx, collectionTuples);
	if(savedTrackTuples.size() > 0) {
		tx.addSQL(String::join(ArrayList<String>{
			"INSERT OR REPLACE INTO SavedTrack (",
			String::join(savedTrackTupleColumns(), ", "),
			") VALUES ",
			String::join(savedTrackTuples, ", ")
		}), savedTrackParams);
	}
	if(savedAlbumTuples.size() > 0) {
		tx.addSQL(String::join(ArrayList<String>{
			"INSERT OR REPLACE INTO SavedAlbum (",
			String::join(savedAlbumTupleColumns(), ", "),
			") VALUES ",
			String::join(savedAlbumTuples, ", ")
		}), savedAlbumParams);
	}
	if(savedPlaylistTuples.size() > 0) {
		tx.addSQL(String::join(ArrayList<String>{
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
		tx.addSQL(String::join(ArrayList<String>{
			"INSERT OR REPLACE DBState (",
			String::join(dbStateTupleColumns(), ", "),
			") VALUES ",
			String::join(dbStateTuples, ", ")
		}), dbStateParams);
	}
}

/*void selectTrack(SQLiteTransaction& tx, String outKey, String uri) {
	//
}

void selectTrackCount(SQLiteTransaction& tx, String outKey);
void selectTrackCollection(SQLiteTransaction& tx, String outKey, String uri);
void selectTrackCollectionItemsAndTracks(SQLiteTransaction& tx, String outKey, String collectionURI, Optional<IndexRange> range);
void selectArtist(SQLiteTransaction& tx, String outKey, String uri);
struct LibraryItemSelectOptions {
	String libraryProvider;
	Optional<IndexRange> range;
	String order;
};
void selectSavedTracks(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options = LibraryItemSelectOptions());
void selectSavedAlbums(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options = LibraryItemSelectOptions());
void selectSavedPlaylists(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options = LibraryItemSelectOptions());
void selectLibraryArtists(SQLiteTransaction& tx, String outKey, LibraryItemSelectOptions options = LibraryItemSelectOptions());
void selectDBState(SQLiteTransaction& tx, String outKey, String stateKey);*/

}

//
//  MediaDatabaseSQL.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 5/25/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "MediaDatabaseSQL.hpp"
#include <any>

namespace sh::sql {

String sqlParam(LinkedList<Any>& params, Any param) {
	params.pushBack(param);
	return "?";
}

String sqlOptParam(LinkedList<Any>& params, Any param, String defaultQuery, ArrayList<Any> defaultParams) {
	if(param.empty()) {
		params.pushBackList(defaultParams);
		return defaultQuery;
	}
	params.pushBack(param);
	return "?";
}

Any sqlStringOrNull(String str) {
	if(str.empty()) {
		return Any();
	}
	return str;
}

#define COALESCE_FIELD(params, coalesce, field, table, item, value) \
	(coalesce ? \
		sqlOptParam(params, value, \
			"(SELECT " field " FROM " table " WHERE uri=?)", { Any(item->uri()) }) \
		: sqlParam(params, value))




String createDB() {
	return R"SQL(
CREATE TABLE IF NOT EXISTS Artist (
	uri TEXT NOT NULL UNIQUE,
	provider TEXT NOT NULL,
	type TEXT NOT NULL,
	name TEXT NOT NULL,
	images TEXT,
	updateTime TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY(uri)
);
CREATE TABLE IF NOT EXISTS TrackCollection (
	uri TEXT NOT NULL UNIQUE,
	provider TEXT NOT NULL,
	type TEXT NOT NULL,
	name TEXT NOT NULL,
	itemCount INT,
	artists TEXT,
	images TEXT,
	updateTime TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY(uri)
);
CREATE TABLE IF NOT EXISTS Track (
	uri text NOT NULL UNIQUE,
	provider text NOT NULL,
	name text NOT NULL,
	albumName TEXT,
	albumURI TEXT,
	artists TEXT,
	images TEXT,
	duration REAL,
	playable INT(1),
	updateTime TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY(uri)
);
CREATE TABLE IF NOT EXISTS TrackCollectionItem (
	collectionURI TEXT NOT NULL,
	indexNum INT NOT NULL,
	trackURI TEXT NOT NULL,
	addedAt TEXT,
	updateTime TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY(collectionURI, indexNum),
	FOREIGN KEY(collectionURI) REFERENCES TrackCollection(uri),
	FOREIGN KEY(trackURI) REFERENCES Track(uri)
);
CREATE TABLE IF NOT EXISTS TrackArtist (
	trackURI TEXT NOT NULL,
	artistURI TEXT NOT NULL,
	updateTime TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY(trackURI, artistURI),
	FOREIGN KEY(trackURI) REFERENCES Track(uri),
	FOREIGN KEY(artistURI) REFERENCES Artist(uri)
);
CREATE TABLE IF NOT EXISTS TrackCollectionArtist (
	collectionURI TEXT NOT NULL,
	artistURI TEXT NOT NULL,
	updateTime TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY(collectionURI, artistURI),
	FOREIGN KEY(collectionURI) REFERENCES TrackCollection(uri),
	FOREIGN KEY(artistURI) REFERENCES Artist(uri)
);
CREATE TABLE IF NOT EXISTS SavedTrack (
	trackURI TEXT NOT NULL,
	libraryProvider TEXT NOT NULL,
	addedAt TEXT,
	updateTime TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY(trackURI, libraryProvider),
	FOREIGN KEY(trackURI) REFERENCES Track(uri)
);
CREATE TABLE IF NOT EXISTS SavedAlbum (
	albumURI TEXT NOT NULL,
	libraryProvider TEXT NOT NULL,
	addedAt TEXT,
	updateTime TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY(albumURI, libraryProvider),
	FOREIGN KEY(albumURI) REFERENCES TrackCollection(uri)
);
CREATE TABLE IF NOT EXISTS SavedPlaylist (
	playlistURI TEXT NOT NULL,
	libraryProvider TEXT NOT NULL,
	addedAt TEXT,
	updateTime TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY(playlistURI, libraryProvider),
	FOREIGN KEY(playlistURI) REFERENCES TrackCollection(uri)
);
)SQL";
}

String purgeDB() {
	return R"SQL(
DROP TABLE IF EXISTS TrackCollection;
DROP TABLE IF EXISTS Track;
DROP TABLE IF EXISTS TrackCollectionItem;
DROP TABLE IF EXISTS Artist;
DROP TABLE IF EXISTS TrackArtist;
DROP TABLE IF EXISTS TrackCollectionArtist;
DROP TABLE IF EXISTS SavedTrack;
DROP TABLE IF EXISTS SavedAlbum;
DROP TABLE IF EXISTS SavedPlaylist;
DROP TABLE IF EXISTS DBState;
)SQL";
}




Any nonEmptyArtistsJson(ArrayList<$<Artist>> artists) {
	if(artists.size() == 0) {
		return Any();
	}
	Json::array arr;
	arr.reserve(artists.size());
	for(auto& artist : artists) {
		arr.push_back(Json::object{
			{ "type", (std::string)artist->type() },
			{ "uri", (std::string)artist->uri() },
			{ "provider", (std::string)artist->mediaProvider()->name() },
			{ "name", (std::string)artist->name() }
		});
	}
	return sqlStringOrNull(Json(arr).dump());
}

Any imagesJson(Optional<ArrayList<MediaItem::Image>> images) {
	if(!images) {
		return Any();
	}
	Json::array arr;
	arr.reserve(images->size());
	for(auto& image : images.value()) {
		arr.push_back(image.toJson());
	}
	return sqlStringOrNull(Json(arr).dump());
}

ArrayList<$<Artist>> collectionArtists($<TrackCollection> collection) {
	if(auto album = std::dynamic_pointer_cast<Album>(collection)) {
		return album->artists();
	}
	return {};
}

String collectionItemAddedAt($<TrackCollectionItem> item) {
	if(auto playlistItem = std::dynamic_pointer_cast<PlaylistItem>(item)) {
		return playlistItem->addedAt();
	}
	return String();
}



ArrayList<String> trackTupleColumns() {
	return { "uri", "provider", "name", "albumName", "albumURI", "artists", "images", "duration", "playable", "updateTime" };
}
String trackTuple(LinkedList<Any>& params, $<Track> track, TupleOptions options) {
	return String::join(ArrayList<String>{ "(",
		// uri
		sqlParam(params, track->uri()),",",
		// provider
		sqlParam(params, track->mediaProvider()->name()),",",
		// name
		sqlParam(params, track->name()),",",
		// albumName
		COALESCE_FIELD(params, options.coalesce, "albumName", "Track", track, sqlStringOrNull(track->albumName())),",",
		// albumURI
		COALESCE_FIELD(params, options.coalesce, "albumURI", "Track", track, sqlStringOrNull(track->albumURI())),",",
		// artists
		COALESCE_FIELD(params, options.coalesce, "artists", "Track", track, nonEmptyArtistsJson(track->artists())),",",
		// images
		COALESCE_FIELD(params, options.coalesce, "images", "Track", track, imagesJson(track->images())),",",
		// duration
		COALESCE_FIELD(params, options.coalesce, "duration", "Track", track, track->duration().toAny()),",",
		// playable
		sqlParam(params, track->isPlayable()),",",
		// updateTime
		"CURRENT_TIMESTAMP",
	")" });
}

ArrayList<String> trackArtistTupleColumns() {
	return { "trackURI", "artistURI", "updateTime" };
}
String trackArtistTuple(LinkedList<Any>& params, TrackArtist trackArtist) {
	return String::join(ArrayList<String>{ "(",
		// trackURI
		sqlParam(params, trackArtist.trackURI),",",
		// artistURI
		sqlParam(params, trackArtist.artistURI),",",
		// updateTime
		"CURRENT_TIMESTAMP",
	")" });
}

ArrayList<String> trackCollectionTupleColumns() {
	return { "uri", "provider", "type", "name", "itemCount", "artists", "images", "updateTime" };
}
String trackCollectionTuple(LinkedList<Any>& params, $<TrackCollection> collection, TupleOptions options) {
	return String::join(ArrayList<String>{ "(",
		// uri
		sqlParam(params, collection->uri()),",",
		// provider
		sqlParam(params, collection->mediaProvider()->name()),",",
		// type
		sqlParam(params, collection->type()),",",
		// name
		sqlParam(params, collection->name()),",",
		// itemCount
		COALESCE_FIELD(params, options.coalesce, "itemCount", "TrackCollection", collection, collection->itemCount().toAny()),",",
		// artists
		COALESCE_FIELD(params, options.coalesce, "artists", "TrackCollection", collection, nonEmptyArtistsJson(collectionArtists(collection))),",",
		// images
		COALESCE_FIELD(params, options.coalesce, "images", "TrackCollection", collection, imagesJson(collection->images())),",",
		// updateTime
		"CURRENT_TIMESTAMP",
	")" });
}

ArrayList<String> trackCollectionItemTupleColumns() {
	return { "collectionURI", "indexNum", "trackURI", "addedAt", "updateTime" };
}
String trackCollectionItemTuple(LinkedList<Any>& params, $<TrackCollectionItem> item, TupleOptions options) {
	auto collection = item->context().lock();
	if(!collection) {
		throw std::invalid_argument("Cannot create track collection item tuple without valid collection");
	}
	auto indexNum = item->indexInContext();
	if(!indexNum) {
		throw std::invalid_argument("Cannot create track collection item tuple with detached item");
	}
	return String::join(ArrayList<String>{ "(",
		// collectionURI
		sqlParam(params, collection->uri()),",",
		// indexNum
		sqlParam(params, indexNum.value()),",",
		// trackURI
		sqlParam(params, item->track()->uri()),",",
		// addedAt
		sqlParam(params, sqlStringOrNull(collectionItemAddedAt(item))),",",
		// updateTime
		"CURRENT_TIMESTAMP",
	")" });
}

ArrayList<String> albumItemTupleFromTrackColumns() {
	return { "collectionURI", "indexNum", "trackURI", "updateTime" };
}
String albumItemTupleFromTrack(LinkedList<Any>& params, $<Track> track, TupleOptions options) {
	auto albumURI = track->albumURI();
	if(albumURI.empty()) {
		throw std::invalid_argument("Cannot create album item tuple without albumURI");
	}
	auto trackNum = track->trackNumber();
	if(!trackNum) {
		throw std::invalid_argument("Cannot create album item tuple without a valid track number");
	} else if(trackNum == 0) {
		throw std::invalid_argument("Cannot create album item tuple with a track number equal to 0");
	}
	return String::join(ArrayList<String>{ "(",
		// collectionURI
		sqlParam(params, albumURI),",",
		// indexNum
		sqlParam(params, trackNum.value() - 1),",",
		// trackURI
		sqlParam(params, track->uri()),",",
		// updateTime
		"CURRENT_TIMESTAMP",
	")" });
}

ArrayList<String> trackCollectionArtistTupleColumns() {
	return { "collectionURI", "artistURI", "updateTime" };
}
String trackCollectionArtistTuple(LinkedList<Any>& params, TrackCollectionArtist collectionArtist) {
	return String::join(ArrayList<String>{ "(",
		// collectionURI
		sqlParam(params, collectionArtist.collectionURI),",",
		// artistURI
		sqlParam(params, collectionArtist.artistURI),",",
		// updateTime
		"CURRENT_TIMESTAMP",
	")" });
}

ArrayList<String> artistTupleColumns() {
	return { "uri", "provider", "type", "name", "images", "updateTime" };
}
String artistTuple(LinkedList<Any>& params, $<Artist> artist, TupleOptions options) {
	return String::join(ArrayList<String>{ "(",
		// uri
		sqlParam(params, artist->uri()),",",
		// provider
		sqlParam(params, artist->mediaProvider()->name()),",",
		// type
		sqlParam(params, artist->type()),",",
		// name
		sqlParam(params, artist->name()),",",
		// images
		COALESCE_FIELD(params, options.coalesce, "images", "Artist", artist, imagesJson(artist->images())),",",
		// updateTime
		"CURRENT_TIMESTAMP",
	")" });
}

ArrayList<String> savedTrackTupleColumns() {
	return { "trackURI", "libraryProvider", "addedAt", "updateTime" };
}
String savedTrackTuple(LinkedList<Any>& params, SavedTrack track, TupleOptions options) {
	return String::join(ArrayList<String>{ "(",
		// trackURI
		sqlParam(params, track.track->uri()),",",
		// libraryProvider
		sqlParam(params, track.libraryProvider->name()),",",
		// addedAt
		sqlParam(params, sqlStringOrNull(track.addedAt)),",",
		// updateTime
		"CURRENT_TIMESTAMP",
	")" });
}

ArrayList<String> savedAlbumTupleColumns() {
	return { "albumURI", "libraryProvider", "addedAt", "updateTime" };
}
String savedAlbumTuple(LinkedList<Any>& params, SavedAlbum album, TupleOptions options) {
	return String::join(ArrayList<String>{ "(",
		// albumURI
		sqlParam(params, album.album->uri()),",",
		// libraryProvider
		sqlParam(params, album.libraryProvider->name()),",",
		// addedAt
		sqlParam(params, sqlStringOrNull(album.addedAt)),",",
		// updateTime
		"CURRENT_TIMESTAMP",
	")" });
}

ArrayList<String> savedPlaylistTupleColumns() {
	return { "playlistURI", "libraryProvider", "addedAt", "updateTime" };
}
String savedPlaylistTuple(LinkedList<Any>& params, SavedPlaylist playlist, TupleOptions options) {
	return String::join(ArrayList<String>{ "(",
		// playlistURI
		sqlParam(params, playlist.playlist->uri()),",",
		// libraryProvider
		sqlParam(params, playlist.libraryProvider->name()),",",
		// addedAt
		sqlParam(params, sqlStringOrNull(playlist.addedAt)),",",
		// updateTime
		"CURRENT_TIMESTAMP",
	")" });
}

ArrayList<String> dbStateTupleColumns() {
	return { "stateKey", "stateValue", "updateTime" };
}
String dbStateTuple(LinkedList<Any>& params, DBState state) {
	return String::join(ArrayList<String>{ "(",
		// stateKey
		sqlParam(params, state.stateKey),",",
		// stateValue
		sqlParam(params, state.stateValue),",",
		// updateTime
		"CURRENT_TIMESTAMP",
	")" });
}

}

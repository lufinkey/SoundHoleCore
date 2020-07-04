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

#define COALESCE_FIELD(params, field, table, uri) \
	sqlOptParam(params, Any(), \
		"(SELECT " field " FROM " table " WHERE uri = ?)", { Any((String)uri) })
#define MAYBE_COALESCE_FIELD(coalesce, params, field, table, item, value) \
	(coalesce ? \
		sqlOptParam(params, value, \
			"(SELECT " field " FROM " table " WHERE uri = ?)", { Any(item->uri()) }) \
		: sqlParam(params, value))



ArrayList<String> artistColumns() {
	return { "uri", "provider", "type", "name", "images", "updateTime" };
}
ArrayList<String> followedArtistColumns() {
	return { "artistURI", "libraryProvider", "updateTime" };
}
ArrayList<String> trackColumns() {
	return { "uri", "provider", "name", "albumName", "albumURI", "artists", "images", "duration", "playable", "updateTime" };
}
ArrayList<String> trackCollectionColumns() {
	return { "uri", "provider", "type", "name", "versionId", "itemCount", "artists", "images", "updateTime" };
}
ArrayList<String> trackCollectionItemColumns() {
	return { "collectionURI", "indexNum", "trackURI", "addedAt", "updateTime" };
}
ArrayList<String> savedTrackColumns() {
	return { "trackURI", "libraryProvider", "addedAt", "updateTime" };
}
ArrayList<String> savedAlbumColumns() {
	return { "albumURI", "libraryProvider", "addedAt", "updateTime" };
}
ArrayList<String> savedPlaylistColumns() {
	return { "playlistURI", "libraryProvider", "addedAt", "updateTime" };
}



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
CREATE TABLE IF NOT EXISTS FollowedArtist (
	artistURI TEXT NOT NULL UNIQUE,
	libraryProvider TEXT NOT NULL UNIQUE,
	updateTime TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY(artistURI, libraryProvider)
);
CREATE TABLE IF NOT EXISTS TrackCollection (
	uri TEXT NOT NULL UNIQUE,
	provider TEXT NOT NULL,
	type TEXT NOT NULL,
	name TEXT NOT NULL,
	versionId TEXT,
	itemCount INT,
	artists TEXT,
	images TEXT,
	updateTime TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY(uri)
);
CREATE TABLE IF NOT EXISTS TrackCollectionArtist (
	collectionURI TEXT NOT NULL,
	artistURI TEXT NOT NULL,
	updateTime TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY(collectionURI, artistURI),
	FOREIGN KEY(collectionURI) REFERENCES TrackCollection(uri),
	FOREIGN KEY(artistURI) REFERENCES Artist(uri)
);
CREATE TABLE IF NOT EXISTS Track (
	uri text NOT NULL UNIQUE,
	provider TEXT NOT NULL,
	name TEXT NOT NULL,
	albumName TEXT,
	albumURI TEXT,
	artists TEXT,
	images TEXT,
	duration REAL,
	playable INT(1),
	updateTime TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY(uri),
	FOREIGN KEY(albumURI) REFERENCES TrackCollection(uri)
);
CREATE TABLE IF NOT EXISTS TrackArtist (
	trackURI TEXT NOT NULL,
	artistURI TEXT NOT NULL,
	updateTime TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY(trackURI, artistURI),
	FOREIGN KEY(trackURI) REFERENCES Track(uri),
	FOREIGN KEY(artistURI) REFERENCES Artist(uri)
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
CREATE TABLE IF NOT EXISTS DBState (
	stateKey TEXT NOT NULL,
	stateValue TEXT NOT NULL,
	updateTime TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY(stateKey)
);
)SQL";
}

String purgeDB() {
	return R"SQL(
DROP TABLE IF EXISTS SavedPlaylist;
DROP TABLE IF EXISTS SavedAlbum;
DROP TABLE IF EXISTS SavedTrack;
DROP TABLE IF EXISTS TrackArtist;
DROP TABLE IF EXISTS TrackCollectionArtist;
DROP TABLE IF EXISTS TrackCollectionItem;
DROP TABLE IF EXISTS Track;
DROP TABLE IF EXISTS TrackCollection;
DROP TABLE IF EXISTS FollowedArtist;
DROP TABLE IF EXISTS Artist;
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

ArrayList<$<Artist>> trackCollectionArtists($<TrackCollection> collection) {
	if(auto album = std::dynamic_pointer_cast<Album>(collection)) {
		return album->artists();
	}
	return {};
}

String trackCollectionItemAddedAt($<TrackCollectionItem> item) {
	if(auto playlistItem = std::dynamic_pointer_cast<PlaylistItem>(item)) {
		return playlistItem->addedAt();
	}
	return String();
}



ArrayList<String> trackTupleColumns() {
	return { "uri", "provider", "name", "albumName", "albumURI", "artists", "images", "duration", "playable", "updateTime" };
}
String trackTuple(LinkedList<Any>& params, $<Track> track, const TupleOptions& options) {
	return String::join({ "(",
		// uri
		sqlParam(params, track->uri()),",",
		// provider
		sqlParam(params, track->mediaProvider()->name()),",",
		// name
		sqlParam(params, track->name()),",",
		// albumName
		MAYBE_COALESCE_FIELD(options.coalesce, params, "albumName", "Track", track, sqlStringOrNull(track->albumName())),",",
		// albumURI
		MAYBE_COALESCE_FIELD(options.coalesce, params, "albumURI", "Track", track, sqlStringOrNull(track->albumURI())),",",
		// artists
		MAYBE_COALESCE_FIELD(options.coalesce, params, "artists", "Track", track, nonEmptyArtistsJson(track->artists())),",",
		// images
		MAYBE_COALESCE_FIELD(options.coalesce, params, "images", "Track", track, imagesJson(track->images())),",",
		// duration
		MAYBE_COALESCE_FIELD(options.coalesce, params, "duration", "Track", track, track->duration().toAny()),",",
		// playable
		sqlParam(params, track->isPlayable()),",",
		// updateTime
		"CURRENT_TIMESTAMP",
	")" });
}

ArrayList<String> albumTupleFromTrackColumns() {
	return { "uri", "provider", "type", "name", "versionId", "itemCount", "artists", "images", "updateTime" };
}
String albumTupleFromTrack(LinkedList<Any>& params, $<Track> track, const TupleOptions& options) {
	if(track->albumURI().empty()) {
		throw std::runtime_error("Cannot create album from track with empty album URI");
	}
	return String::join({ "(",
		// uri
		sqlParam(params, track->albumURI()),",",
		// provider
		sqlParam(params, track->mediaProvider()->name()),",",
		// type
		sqlParam(params, String("album")),",",
		// name
		sqlParam(params, track->albumName()),",",
		// versionId
		COALESCE_FIELD(params, "versionId", "TrackCollection", track->albumURI()),",",
		// itemCount
		COALESCE_FIELD(params, "itemCount", "TrackCollection", track->albumURI()),",",
		// artists
		COALESCE_FIELD(params, "artists", "TrackCollection", track->albumURI()),",",
		// images
		COALESCE_FIELD(params, "images", "TrackCollection", track->albumURI()),",",
		// updateTime
		"CURRENT_TIMESTAMP",
	")" });
}

ArrayList<String> trackArtistTupleColumns() {
	return { "trackURI", "artistURI", "updateTime" };
}
String trackArtistTuple(LinkedList<Any>& params, const TrackArtist& trackArtist) {
	return String::join({ "(",
		// trackURI
		sqlParam(params, trackArtist.trackURI),",",
		// artistURI
		sqlParam(params, trackArtist.artistURI),",",
		// updateTime
		"CURRENT_TIMESTAMP",
	")" });
}

ArrayList<String> trackCollectionTupleColumns() {
	return { "uri", "provider", "type", "name", "versionId", "itemCount", "artists", "images", "updateTime" };
}
String trackCollectionTuple(LinkedList<Any>& params, $<TrackCollection> collection, const TrackCollectionTupleOptions& options) {
	return String::join({ "(",
		// uri
		sqlParam(params, collection->uri()),",",
		// provider
		sqlParam(params, collection->mediaProvider()->name()),",",
		// type
		sqlParam(params, collection->type()),",",
		// name
		sqlParam(params, collection->name()),",",
		// versionId
		options.updateVersionId ? (
			sqlParam(params, collection->versionId())
		) : (
			COALESCE_FIELD(params, "versionId", "TrackCollection", collection->uri())
		),",",
		// itemCount
		MAYBE_COALESCE_FIELD(options.coalesce, params, "itemCount", "TrackCollection", collection, collection->itemCount().toAny()),",",
		// artists
		MAYBE_COALESCE_FIELD(options.coalesce, params, "artists", "TrackCollection", collection, nonEmptyArtistsJson(trackCollectionArtists(collection))),",",
		// images
		MAYBE_COALESCE_FIELD(options.coalesce, params, "images", "TrackCollection", collection, imagesJson(collection->images())),",",
		// updateTime
		"CURRENT_TIMESTAMP",
	")" });
}

ArrayList<String> trackCollectionItemTupleColumns() {
	return { "collectionURI", "indexNum", "trackURI", "addedAt", "updateTime" };
}
String trackCollectionItemTuple(LinkedList<Any>& params, $<TrackCollectionItem> item) {
	auto collection = item->context().lock();
	if(!collection) {
		throw std::invalid_argument("Cannot create track collection item tuple without valid collection");
	}
	auto indexNum = item->indexInContext();
	if(!indexNum) {
		throw std::invalid_argument("Cannot create track collection item tuple with detached item");
	}
	return String::join({ "(",
		// collectionURI
		sqlParam(params, collection->uri()),",",
		// indexNum
		sqlParam(params, indexNum.value()),",",
		// trackURI
		sqlParam(params, item->track()->uri()),",",
		// addedAt
		sqlParam(params, sqlStringOrNull(trackCollectionItemAddedAt(item))),",",
		// updateTime
		"CURRENT_TIMESTAMP",
	")" });
}

ArrayList<String> albumItemTupleFromTrackColumns() {
	return { "collectionURI", "indexNum", "trackURI", "updateTime" };
}
String albumItemTupleFromTrack(LinkedList<Any>& params, $<Track> track) {
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
	return String::join({ "(",
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
String trackCollectionArtistTuple(LinkedList<Any>& params, const TrackCollectionArtist& collectionArtist) {
	return String::join({ "(",
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
String artistTuple(LinkedList<Any>& params, $<Artist> artist, const TupleOptions& options) {
	return String::join({ "(",
		// uri
		sqlParam(params, artist->uri()),",",
		// provider
		sqlParam(params, artist->mediaProvider()->name()),",",
		// type
		sqlParam(params, artist->type()),",",
		// name
		sqlParam(params, artist->name()),",",
		// images
		MAYBE_COALESCE_FIELD(options.coalesce, params, "images", "Artist", artist, imagesJson(artist->images())),",",
		// updateTime
		"CURRENT_TIMESTAMP",
	")" });
}

ArrayList<String> followedArtistTupleColumns() {
	return { "artistURI", "libraryProvider", "updateTime" };
}
String followedArtistTuple(LinkedList<Any>& params, const FollowedArtist& followedArtist) {
	return String::join({ "(",
		// artistURI
		sqlParam(params, followedArtist.artistURI),",",
		// libraryProvider
		sqlParam(params, followedArtist.libraryProvider),",",
		// updateTime
		"CURRENT_TIMESTAMP",
	")" });
}

ArrayList<String> savedTrackTupleColumns() {
	return { "trackURI", "libraryProvider", "addedAt", "updateTime" };
}
String savedTrackTuple(LinkedList<Any>& params, const SavedTrack& track) {
	return String::join({ "(",
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
String savedAlbumTuple(LinkedList<Any>& params, const SavedAlbum& album) {
	return String::join({ "(",
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
String savedPlaylistTuple(LinkedList<Any>& params, const SavedPlaylist& playlist) {
	return String::join({ "(",
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
String dbStateTuple(LinkedList<Any>& params, const DBState& state) {
	return String::join({ "(",
		// stateKey
		sqlParam(params, state.stateKey),",",
		// stateValue
		sqlParam(params, state.stateValue),",",
		// updateTime
		"CURRENT_TIMESTAMP",
	")" });
}

}

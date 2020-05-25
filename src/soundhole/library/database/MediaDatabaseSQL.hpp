//
//  MediaDatabaseSQL.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 5/25/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/Track.hpp>
#include <soundhole/media/MediaProvider.hpp>

namespace sh::sql {

String createDB();
String purgeDB();

struct TupleOptions {
	bool coalesce = false;
};

ArrayList<String> trackTupleColumns();
String trackTuple(LinkedList<Any>& params, $<Track> track, TupleOptions options);

ArrayList<String> albumTupleFromTrackColumns();
String albumTupleFromTrack(LinkedList<Any>& params, $<Track> track, TupleOptions options);

struct TrackArtist {
	String trackURI;
	String artistURI;
};
ArrayList<String> trackArtistTupleColumns();
String trackArtistTuple(LinkedList<Any>& params, TrackArtist trackArtist);

ArrayList<String> trackCollectionTupleColumns();
String trackCollectionTuple(LinkedList<Any>& params, $<TrackCollection> collection, TupleOptions options);
ArrayList<String> trackCollectionItemTupleColumns();
String trackCollectionItemTuple(LinkedList<Any>& params, $<TrackCollectionItem> item, TupleOptions options);
ArrayList<String> albumItemTupleFromTrackColumns();
String albumItemTupleFromTrack(LinkedList<Any>& params, $<Track> track, TupleOptions options);

struct TrackCollectionArtist {
	String collectionURI;
	String artistURI;
};
ArrayList<String> trackCollectionArtistTupleColumns();
String trackCollectionArtistTuple(LinkedList<Any>& params, TrackCollectionArtist collectionArtist);

ArrayList<String> artistTupleColumns();
String artistTuple(LinkedList<Any>& params, $<Artist> artist, TupleOptions options);

struct SavedTrack {
	$<Track> track;
	MediaProvider* libraryProvider;
	String addedAt;
};
ArrayList<String> savedTrackTupleColumns();
String savedTrackTuple(LinkedList<Any>& params, SavedTrack track, TupleOptions options);

struct SavedAlbum {
	$<Album> album;
	MediaProvider* libraryProvider;
	String addedAt;
};
ArrayList<String> savedAlbumTupleColumns();
String savedAlbumTuple(LinkedList<Any>& params, SavedAlbum album, TupleOptions options);

struct SavedPlaylist {
	$<Playlist> playlist;
	MediaProvider* libraryProvider;
	String addedAt;
};
ArrayList<String> savedPlaylistTupleColumns();
String savedPlaylistTuple(LinkedList<Any>& params, SavedPlaylist playlist, TupleOptions options);

struct DBState {
	String stateKey;
	String stateValue;
};
ArrayList<String> dbStateTupleColumns();
String dbStateTuple(LinkedList<Any>& params, DBState state);

}

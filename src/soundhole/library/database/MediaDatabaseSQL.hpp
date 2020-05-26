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
#include <soundhole/media/TrackCollection.hpp>
#include <soundhole/media/Album.hpp>
#include <soundhole/media/Playlist.hpp>
#include <soundhole/media/Artist.hpp>
#include <soundhole/media/MediaProvider.hpp>

namespace sh::sql {

ArrayList<$<Artist>> trackCollectionArtists($<TrackCollection> collection);
String trackCollectionItemAddedAt($<TrackCollectionItem> item);

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
String trackCollectionItemTuple(LinkedList<Any>& params, $<TrackCollectionItem> item);
ArrayList<String> albumItemTupleFromTrackColumns();
String albumItemTupleFromTrack(LinkedList<Any>& params, $<Track> track);

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
String savedTrackTuple(LinkedList<Any>& params, SavedTrack track);

struct SavedAlbum {
	$<Album> album;
	MediaProvider* libraryProvider;
	String addedAt;
};
ArrayList<String> savedAlbumTupleColumns();
String savedAlbumTuple(LinkedList<Any>& params, SavedAlbum album);

struct SavedPlaylist {
	$<Playlist> playlist;
	MediaProvider* libraryProvider;
	String addedAt;
};
ArrayList<String> savedPlaylistTupleColumns();
String savedPlaylistTuple(LinkedList<Any>& params, SavedPlaylist playlist);

struct DBState {
	String stateKey;
	String stateValue;
};
ArrayList<String> dbStateTupleColumns();
String dbStateTuple(LinkedList<Any>& params, DBState state);

}

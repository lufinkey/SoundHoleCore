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

String sqlParam(LinkedList<Any>& params, Any param);
String sqlOptParam(LinkedList<Any>& params, Any param, String defaultQuery, ArrayList<Any> defaultParams);
Any sqlStringOrNull(String str);

ArrayList<String> artistColumns();
ArrayList<String> followedArtistColumns();
ArrayList<String> trackColumns();
ArrayList<String> trackCollectionColumns();
ArrayList<String> trackCollectionItemColumns();
ArrayList<String> savedTrackColumns();
ArrayList<String> savedAlbumColumns();
ArrayList<String> savedPlaylistColumns();

String createDB();
String purgeDB();

struct TupleOptions {
	bool coalesce = false;
};
struct TrackCollectionTupleOptions {
	bool coalesce = false;
	bool updateVersionId = false;
};

ArrayList<String> trackTupleColumns();
String trackTuple(LinkedList<Any>& params, $<Track> track, const TupleOptions& options);

ArrayList<String> albumTupleFromTrackColumns();
String albumTupleFromTrack(LinkedList<Any>& params, $<Track> track, const TupleOptions& options);

struct TrackArtist {
	String trackURI;
	String artistURI;
};
ArrayList<String> trackArtistTupleColumns();
String trackArtistTuple(LinkedList<Any>& params, const TrackArtist& trackArtist);

Optional<ArrayList<$<Artist>>> trackCollectionArtists($<TrackCollection> collection);
$<UserAccount> trackCollectionOwner($<TrackCollection> collection);

ArrayList<String> trackCollectionTupleColumns();
String trackCollectionTuple(LinkedList<Any>& params, $<TrackCollection> collection, const TrackCollectionTupleOptions& options);
ArrayList<String> trackCollectionItemTupleColumns();
String trackCollectionItemTuple(LinkedList<Any>& params, $<TrackCollectionItem> item);
ArrayList<String> albumItemTupleFromTrackColumns();
String albumItemTupleFromTrack(LinkedList<Any>& params, $<Track> track);

struct TrackCollectionArtist {
	String collectionURI;
	String artistURI;
};
ArrayList<String> trackCollectionArtistTupleColumns();
String trackCollectionArtistTuple(LinkedList<Any>& params, const TrackCollectionArtist& collectionArtist);

ArrayList<String> artistTupleColumns();
String artistTuple(LinkedList<Any>& params, $<Artist> artist, const TupleOptions& options);

struct FollowedArtist {
	String artistURI;
	String libraryProvider;
};
ArrayList<String> followedArtistTupleColumns();
String followedArtistTuple(LinkedList<Any>& params, const FollowedArtist& followedArtist);

struct SavedTrack {
	$<Track> track;
	MediaProvider* libraryProvider;
	String addedAt;
};
ArrayList<String> savedTrackTupleColumns();
String savedTrackTuple(LinkedList<Any>& params, const SavedTrack& track);

struct SavedAlbum {
	$<Album> album;
	MediaProvider* libraryProvider;
	String addedAt;
};
ArrayList<String> savedAlbumTupleColumns();
String savedAlbumTuple(LinkedList<Any>& params, const SavedAlbum& album);

struct SavedPlaylist {
	$<Playlist> playlist;
	MediaProvider* libraryProvider;
	String addedAt;
};
ArrayList<String> savedPlaylistTupleColumns();
String savedPlaylistTuple(LinkedList<Any>& params, const SavedPlaylist& playlist);

struct DBState {
	String stateKey;
	String stateValue;
};
ArrayList<String> dbStateTupleColumns();
String dbStateTuple(LinkedList<Any>& params, const DBState& state);

}

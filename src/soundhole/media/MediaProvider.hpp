//
//  MediaProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "MediaPlaybackProvider.hpp"
#include "MediaItem.hpp"
#include "Artist.hpp"
#include "Track.hpp"
#include "Album.hpp"
#include "Playlist.hpp"
#include "UserAccount.hpp"

namespace sh {
	class MediaProvider {
	public:
		template<typename T>
		struct LoadBatch {
			LinkedList<T> items;
			Optional<size_t> total;
		};
		
		using ArtistAlbumsGenerator = ContinuousGenerator<LoadBatch<$<Album>>,void>;
		using UserPlaylistsGenerator = ContinuousGenerator<LoadBatch<$<Playlist>>,void>;
		
		MediaProvider(const MediaProvider&) = delete;
		MediaProvider& operator=(const MediaProvider&) = delete;
		
		MediaProvider() {};
		virtual ~MediaProvider() {}
		
		virtual String name() const = 0;
		virtual String displayName() const = 0;
		
		virtual Promise<bool> login() = 0;
		virtual void logout() = 0;
		virtual bool isLoggedIn() const = 0;
		
		virtual Promise<Track::Data> getTrackData(String uri) = 0;
		virtual Promise<Artist::Data> getArtistData(String uri) = 0;
		virtual Promise<Album::Data> getAlbumData(String uri) = 0;
		virtual Promise<Playlist::Data> getPlaylistData(String uri) = 0;
		virtual Promise<UserAccount::Data> getUserData(String uri) = 0;
		
		virtual Promise<$<Track>> getTrack(String uri);
		virtual Promise<$<Artist>> getArtist(String uri);
		virtual Promise<$<Album>> getAlbum(String uri);
		virtual Promise<$<Playlist>> getPlaylist(String uri);
		virtual Promise<$<UserAccount>> getUser(String uri);
		
		virtual Promise<ArrayList<$<Track>>> getArtistTopTracks(String artistURI) = 0;
		virtual ArtistAlbumsGenerator getArtistAlbums(String artistURI) = 0;
		
		virtual UserPlaylistsGenerator getUserPlaylists(String userURI) = 0;
		
		virtual Album::MutatorDelegate* createAlbumMutatorDelegate($<Album> album) = 0;
		virtual Playlist::MutatorDelegate* createPlaylistMutatorDelegate($<Playlist> playlist) = 0;
		
		virtual MediaPlaybackProvider* player() = 0;
		virtual const MediaPlaybackProvider* player() const = 0;
	};
}

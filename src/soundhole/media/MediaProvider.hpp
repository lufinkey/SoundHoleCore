//
//  MediaProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "NamedProvider.hpp"
#include "AuthedProvider.hpp"
#include "MediaPlaybackProvider.hpp"
#include "MediaItem.hpp"
#include "Artist.hpp"
#include "Track.hpp"
#include "Album.hpp"
#include "Playlist.hpp"
#include "UserAccount.hpp"

namespace sh {
	class MediaProviderStash;

	class MediaProvider: public NamedProvider, public AuthedProvider {
	public:
		struct LibraryItem {
			MediaProvider* libraryProvider;
			$<MediaItem> mediaItem;
			String addedAt;
			
			static LibraryItem fromJson(Json json, MediaProviderStash* stash);
		};
		
		template<typename T>
		struct LoadBatch {
			LinkedList<T> items;
			Optional<size_t> total;
		};
		
		struct GenerateLibraryResults {
			Json resumeData;
			LinkedList<LibraryItem> items;
			double progress;
		};
		
		using ArtistAlbumsGenerator = ContinuousGenerator<LoadBatch<$<Album>>,void>;
		using UserPlaylistsGenerator = ContinuousGenerator<LoadBatch<$<Playlist>>,void>;
		using LibraryItemGenerator = ContinuousGenerator<GenerateLibraryResults,void>;
		
		MediaProvider(const MediaProvider&) = delete;
		MediaProvider& operator=(const MediaProvider&) = delete;
		
		MediaProvider() {};
		virtual ~MediaProvider() {}
		
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
		
		virtual bool hasLibrary() const = 0;
		struct GenerateLibraryOptions {
			Json resumeData;
		};
		virtual LibraryItemGenerator generateLibrary(GenerateLibraryOptions options = GenerateLibraryOptions()) = 0;
		
		virtual bool canCreatePlaylists() const;
		virtual ArrayList<Playlist::Privacy> supportedPlaylistPrivacies() const;
		struct CreatePlaylistOptions {
			Playlist::Privacy privacy = Playlist::Privacy::UNKNOWN;
		};
		virtual Promise<$<Playlist>> createPlaylist(String name, CreatePlaylistOptions options);
		virtual Promise<bool> isPlaylistEditable($<Playlist> playlist);
		
		virtual MediaPlaybackProvider* player() = 0;
		virtual const MediaPlaybackProvider* player() const = 0;
		
		virtual $<Track> track(const Track::Data& data);
		virtual $<Artist> artist(const Artist::Data& data);
		virtual $<Album> album(const Album::Data& data);
		virtual $<Playlist> playlist(const Playlist::Data& data);
		virtual $<UserAccount> userAccount(const UserAccount::Data& data);
	};
}

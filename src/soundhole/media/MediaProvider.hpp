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
			$<MediaItem> mediaItem;
			Date addedAt;
			
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
		
		virtual Promise<ArrayList<Track::Data>> getTracksData(ArrayList<String> uris);
		virtual Promise<ArrayList<Artist::Data>> getArtistsData(ArrayList<String> uris);
		virtual Promise<ArrayList<Album::Data>> getAlbumsData(ArrayList<String> uris);
		virtual Promise<ArrayList<Playlist::Data>> getPlaylistsData(ArrayList<String> uris);
		virtual Promise<ArrayList<UserAccount::Data>> getUsersData(ArrayList<String> uris);
		
		Promise<$<Track>> getTrack(String uri);
		Promise<$<Artist>> getArtist(String uri);
		Promise<$<Album>> getAlbum(String uri);
		Promise<$<Playlist>> getPlaylist(String uri);
		Promise<$<UserAccount>> getUser(String uri);
		
		Promise<ArrayList<$<Track>>> getTracks(ArrayList<String> uris);
		Promise<ArrayList<$<Artist>>> getArtists(ArrayList<String> uris);
		Promise<ArrayList<$<Album>>> getAlbums(ArrayList<String> uris);
		Promise<ArrayList<$<Playlist>>> getPlaylists(ArrayList<String> uris);
		Promise<ArrayList<$<UserAccount>>> getUsers(ArrayList<String> uris);
		
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
		
		virtual bool handlesUsersAsArtists() const;
		
		virtual bool canFollowArtists() const = 0;
		virtual Promise<void> followArtist(String artistURI) = 0;
		virtual Promise<void> unfollowArtist(String artistURI) = 0;
		
		virtual bool canFollowUsers() const = 0;
		virtual Promise<void> followUser(String userURI) = 0;
		virtual Promise<void> unfollowUser(String userURI) = 0;
		
		virtual bool canSaveTracks() const = 0;
		virtual Promise<void> saveTrack(String trackURI) = 0;
		virtual Promise<void> unsaveTrack(String trackURI) = 0;
		
		virtual bool canSaveAlbums() const = 0;
		virtual Promise<void> saveAlbum(String albumURI) = 0;
		virtual Promise<void> unsaveAlbum(String albumURI) = 0;
		
		virtual bool canSavePlaylists() const = 0;
		virtual Promise<void> savePlaylist(String playlistURI) = 0;
		virtual Promise<void> unsavePlaylist(String playlistURI) = 0;
		
		virtual bool canCreatePlaylists() const;
		virtual ArrayList<Playlist::Privacy> supportedPlaylistPrivacies() const;
		
		struct CreatePlaylistOptions {
			Playlist::Privacy privacy = Playlist::Privacy::PRIVATE;
			
			Json toJson() const;
			#ifdef NODE_API_MODULE
			Napi::Object toNapiObject(napi_env) const;
			#endif
		};
		virtual Promise<$<Playlist>> createPlaylist(String name, CreatePlaylistOptions options);
		virtual Promise<bool> isPlaylistEditable($<Playlist> playlist);
		virtual Promise<void> deletePlaylist(String playlistURI);
		
		
		virtual MediaPlaybackProvider* player();
		virtual const MediaPlaybackProvider* player() const;
		
		virtual $<Track> track(const Track::Data& data);
		virtual $<Artist> artist(const Artist::Data& data);
		virtual $<Album> album(const Album::Data& data);
		virtual $<Playlist> playlist(const Playlist::Data& data);
		virtual $<UserAccount> userAccount(const UserAccount::Data& data);
	};
}

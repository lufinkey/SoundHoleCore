//
//  SpotifyMediaProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <chrono>
#include <soundhole/common.hpp>
#include <soundhole/media/MediaProvider.hpp>
#include <soundhole/media/AuthedProviderIdentityStore.hpp>
#include "SpotifyPlaybackProvider.hpp"
#include "api/Spotify.hpp"

namespace sh {
	class SpotifyMediaProvider: public MediaProvider, public AuthedProviderIdentityStore<SpotifyUser> {
		friend class SpotifyPlaybackProvider;
		friend class SpotifyAlbumMutatorDelegate;
		friend class SpotifyPlaylistMutatorDelegate;
	public:
		static constexpr auto NAME = "spotify";
		using Options = Spotify::Options;
		
		SpotifyMediaProvider(Options options);
		virtual ~SpotifyMediaProvider();
		
		virtual String name() const override;
		virtual String displayName() const override;
		
		Spotify* api();
		const Spotify* api() const;
		
		virtual Promise<bool> login() override;
		virtual void logout() override;
		bool isLoggedIn() const override;
		virtual Promise<ArrayList<String>> getCurrentUserURIs() override;
		
		struct SearchOptions {
			ArrayList<String> types;
			Optional<size_t> limit;
			Optional<size_t> offset;
		};
		struct SearchResults {
			Optional<SpotifyPage<$<Track>>> tracks;
			Optional<SpotifyPage<$<Album>>> albums;
			Optional<SpotifyPage<$<Artist>>> artists;
			Optional<SpotifyPage<$<Playlist>>> playlists;
		};
		Promise<SearchResults> search(String query, SearchOptions options = {});
		
		virtual Promise<Track::Data> getTrackData(String uri) override;
		virtual Promise<Artist::Data> getArtistData(String uri) override;
		virtual Promise<Album::Data> getAlbumData(String uri) override;
		virtual Promise<Playlist::Data> getPlaylistData(String uri) override;
		virtual Promise<UserAccount::Data> getUserData(String uri) override;
		
		virtual Promise<ArrayList<Track::Data>> getTracksData(ArrayList<String> uris) override;
		virtual Promise<ArrayList<Artist::Data>> getArtistsData(ArrayList<String> uris) override;
		virtual Promise<ArrayList<Album::Data>> getAlbumsData(ArrayList<String> uris) override;
		
		virtual Promise<ArrayList<$<Track>>> getArtistTopTracks(String artistURI) override;
		virtual ArtistAlbumsGenerator getArtistAlbums(String artistURI) override;
		virtual UserPlaylistsGenerator getUserPlaylists(String userURI) override;
		
		virtual Album::MutatorDelegate* createAlbumMutatorDelegate($<Album> album) override;
		virtual Playlist::MutatorDelegate* createPlaylistMutatorDelegate($<Playlist> playlist) override;
		
		
		virtual bool hasLibrary() const override;
		struct GenerateLibraryResumeData {
			struct Item {
				String uri;
				Date addedAt;
				
				Json toJson() const;
				static Optional<Item> maybeFromJson(const Json&);
			};
			
			String userId;
			
			Optional<Date> mostRecentTrackSave;
			Optional<Date> mostRecentAlbumSave;
			
			String syncCurrentType;
			Optional<Date> syncMostRecentSave;
			Optional<size_t> syncLastItemOffset;
			Optional<Item> syncLastItem;
			
			Json toJson() const;
			static GenerateLibraryResumeData fromJson(const Json&);
		};
		
		virtual LibraryItemGenerator generateLibrary(GenerateLibraryOptions options = GenerateLibraryOptions()) override;
		
		virtual bool canFollowArtists() const override;
		virtual Promise<void> followArtist(String artistURI) override;
		virtual Promise<void> unfollowArtist(String artistURI) override;
		
		virtual bool canFollowUsers() const override;
		virtual Promise<void> followUser(String userURI) override;
		virtual Promise<void> unfollowUser(String userURI) override;
		
		virtual bool canSaveTracks() const override;
		virtual Promise<void> saveTrack(String trackURI) override;
		virtual Promise<void> unsaveTrack(String trackURI) override;
		
		virtual bool canSaveAlbums() const override;
		virtual Promise<void> saveAlbum(String albumURI) override;
		virtual Promise<void> unsaveAlbum(String albumURI) override;
		
		virtual bool canSavePlaylists() const override;
		virtual Promise<void> savePlaylist(String playlistURI) override;
		virtual Promise<void> unsavePlaylist(String playlistURI) override;
		
		
		virtual bool canCreatePlaylists() const override;
		virtual ArrayList<Playlist::Privacy> supportedPlaylistPrivacies() const override;
		virtual Promise<$<Playlist>> createPlaylist(String name, CreatePlaylistOptions options) override;
		virtual Promise<bool> isPlaylistEditable($<Playlist> playlist) override;
		virtual Promise<void> deletePlaylist(String playlistURI) override;
		
		
		virtual SpotifyPlaybackProvider* player() override;
		virtual const SpotifyPlaybackProvider* player() const override;
		
		Track::Data createTrackData(SpotifyTrack track, bool partial);
		Artist::Data createArtistData(SpotifyArtist artist, bool partial);
		Album::Data createAlbumData(SpotifyAlbum album, bool partial);
		Playlist::Data createPlaylistData(SpotifyPlaylist playlist, bool partial);
		PlaylistItem::Data createPlaylistItemData(SpotifyPlaylist::Item playlistItem);
		UserAccount::Data createUserAccountData(SpotifyUser user, bool partial);
		static MediaItem::Image createImage(SpotifyImage image);
		
		struct URI {
			String provider;
			String type;
			String id;
		};
		URI parseURI(String uri) const;
		
	protected:
		virtual Promise<Optional<SpotifyUser>> fetchIdentity() override;
		virtual String getIdentityFilePath() const override;
		
	private:
		Spotify* spotify;
		SpotifyPlaybackProvider* _player;
	};
}

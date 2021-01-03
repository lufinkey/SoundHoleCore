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
		virtual Promise<ArrayList<String>> getCurrentUserIds() override;
		
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
		
		virtual Promise<ArrayList<$<Track>>> getArtistTopTracks(String artistURI) override;
		virtual ArtistAlbumsGenerator getArtistAlbums(String artistURI) override;
		
		virtual UserPlaylistsGenerator getUserPlaylists(String userURI) override;
		
		virtual Album::MutatorDelegate* createAlbumMutatorDelegate($<Album> album) override;
		virtual Playlist::MutatorDelegate* createPlaylistMutatorDelegate($<Playlist> playlist) override;
		
		virtual bool hasLibrary() const override;
		struct GenerateLibraryResumeData {
			struct Item {
				String uri;
				String addedAt;
				
				Json toJson() const;
				static Optional<Item> maybeFromJson(const Json&);
			};
			
			String userId;
			
			Optional<time_t> mostRecentTrackSave;
			Optional<time_t> mostRecentAlbumSave;
			
			String syncCurrentType;
			Optional<time_t> syncMostRecentSave;
			Optional<size_t> syncLastItemOffset;
			Optional<Item> syncLastItem;
			
			Json toJson() const;
			
			static GenerateLibraryResumeData fromJson(const Json&);
			static String typeFromSyncIndex(size_t index);
			static Optional<size_t> syncIndexFromType(String type);
		};
		virtual LibraryItemGenerator generateLibrary(GenerateLibraryOptions options = GenerateLibraryOptions()) override;
		
		virtual bool canCreatePlaylists() const override;
		virtual ArrayList<Playlist::Privacy> supportedPlaylistPrivacies() const override;
		virtual Promise<$<Playlist>> createPlaylist(String name, CreatePlaylistOptions options) override;
		virtual Promise<bool> isPlaylistEditable($<Playlist> playlist) override;
		
		virtual SpotifyPlaybackProvider* player() override;
		virtual const SpotifyPlaybackProvider* player() const override;
		
		Track::Data createTrackData(SpotifyTrack track, bool partial);
		Artist::Data createArtistData(SpotifyArtist artist, bool partial);
		Album::Data createAlbumData(SpotifyAlbum album, bool partial);
		Playlist::Data createPlaylistData(SpotifyPlaylist playlist, bool partial);
		PlaylistItem::Data createPlaylistItemData(SpotifyPlaylist::Item playlistItem);
		UserAccount::Data createUserAccountData(SpotifyUser user, bool partial);
		
		struct URI {
			String provider;
			String type;
			String id;
		};
		URI parseURI(String uri);
		
	protected:
		virtual Promise<Optional<SpotifyUser>> fetchIdentity() override;
		virtual String getIdentityFilePath() const override;
		
	private:
		static time_t timeFromString(String time);

		static MediaItem::Image createImage(SpotifyImage image);
		
		Spotify* spotify;
		SpotifyPlaybackProvider* _player;
	};
}

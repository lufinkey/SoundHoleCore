//
//  YoutubeMediaProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/23/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/MediaProvider.hpp>
#include <soundhole/media/AuthedProviderIdentityStore.hpp>
#include "YoutubePlaybackProvider.hpp"
#include "api/Youtube.hpp"

namespace sh {
	struct YoutubeMediaProviderIdentity {
		ArrayList<YoutubeChannel> channels;
		String libraryTracksPlaylistId;
		
		Json toJson() const;
		static YoutubeMediaProviderIdentity fromJson(const Json&);
	};


	class YoutubeMediaProvider: public MediaProvider, public AuthedProviderIdentityStore<YoutubeMediaProviderIdentity> {
		friend class YoutubePlaylistMutatorDelegate;
	public:
		static constexpr auto NAME = "youtube";
		
		struct Options {
			YoutubeAuth::Options auth;
			String libraryPlaylistName;
			String libraryPlaylistDescription;
		};
		
		YoutubeMediaProvider(Options options, StreamPlayer* streamPlayer);
		virtual ~YoutubeMediaProvider();
		
		virtual String name() const override;
		virtual String displayName() const override;
		
		Youtube* api();
		const Youtube* api() const;
		
		virtual Promise<bool> login() override;
		virtual void logout() override;
		virtual bool isLoggedIn() const override;
		virtual Promise<ArrayList<String>> getCurrentUserURIs() override;
		
		using SearchOptions = Youtube::SearchOptions;
		Promise<YoutubePage<$<MediaItem>>> search(String query, SearchOptions options);
		
		virtual Promise<Track::Data> getTrackData(String uri) override;
		virtual Promise<Artist::Data> getArtistData(String uri) override;
		virtual Promise<Album::Data> getAlbumData(String uri) override;
		virtual Promise<Playlist::Data> getPlaylistData(String uri) override;
		virtual Promise<UserAccount::Data> getUserData(String uri) override;
		
		virtual Promise<ArrayList<Track::Data>> getTracksData(ArrayList<String> uris) override;
		virtual Promise<ArrayList<Artist::Data>> getArtistsData(ArrayList<String> uris) override;
		virtual Promise<ArrayList<Album::Data>> getAlbumsData(ArrayList<String> uris) override;
		virtual Promise<ArrayList<Playlist::Data>> getPlaylistsData(ArrayList<String> uris) override;
		virtual Promise<ArrayList<UserAccount::Data>> getUsersData(ArrayList<String> uris) override;
		
		virtual Promise<ArrayList<$<Track>>> getArtistTopTracks(String artistURI) override;
		virtual ArtistAlbumsGenerator getArtistAlbums(String artistURI) override;
		
		virtual UserPlaylistsGenerator getUserPlaylists(String userURI) override;
		
		virtual Album::MutatorDelegate* createAlbumMutatorDelegate($<Album> album) override;
		virtual Playlist::MutatorDelegate* createPlaylistMutatorDelegate($<Playlist> playlist) override;
		
		virtual bool hasLibrary() const override;
		virtual bool handlesUsersAsArtists() const override;
		struct GenerateLibraryResumeData {
			String tracksPlaylistId;
			Optional<time_t> mostRecentTrackSave;
			
			String syncCurrentType;
			Optional<time_t> syncMostRecentSave;
			String syncPageToken;
			size_t syncOffset;
			
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
		
		virtual Promise<void> saveTrack(String trackURI) override;
		virtual Promise<void> unsaveTrack(String trackURI) override;
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
		
		virtual YoutubePlaybackProvider* player() override;
		virtual const YoutubePlaybackProvider* player() const override;
		
		Track::Data createTrackData(YoutubeVideo video);
		Track::Data createTrackData(YoutubeVideoInfo video);
		Artist::Data createArtistData(YoutubeChannel channel);
		Playlist::Data createPlaylistData(YoutubePlaylist playlist);
		PlaylistItem::Data createPlaylistItemData(YoutubePlaylistItem playlistItem);
		UserAccount::Data createUserData(YoutubeChannel channel);
		$<MediaItem> createMediaItem(YoutubeSearchResult searchResult);
		
		struct URI {
			String provider;
			String type;
			String id;
		};
		URI parseURI(String uri) const;
		
	protected:
		virtual Promise<Optional<YoutubeMediaProviderIdentity>> fetchIdentity() override;
		virtual String getIdentityFilePath() const override;
		
		Promise<String> getLibraryTracksPlaylistID();
		Promise<String> fetchLibraryTracksPlaylistID(String pageToken = String());
		
	private:
		static Date dateFromString(String time);
		
		String createURI(String type, String id) const;
		URI parseURL(String url) const;
		
		static MediaItem::Image createImage(YoutubeImage image);
		
		Youtube* youtube;
		YoutubePlaybackProvider* _player;
		String libraryPlaylistName;
		String libraryPlaylistDescription;
	};
}

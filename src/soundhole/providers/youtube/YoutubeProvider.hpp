//
//  YoutubeProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/23/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/MediaProvider.hpp>
#include "YoutubePlaybackProvider.hpp"
#include "api/Youtube.hpp"

namespace sh {
	class YoutubeProvider: public MediaProvider {
		friend class YoutubePlaylistMutatorDelegate;
	public:
		using Options = Youtube::Options;
		YoutubeProvider(Options options);
		virtual ~YoutubeProvider();
		
		virtual String name() const override;
		virtual String displayName() const override;
		
		Youtube* api();
		const Youtube* api() const;
		
		virtual Promise<bool> login() override;
		virtual void logout() override;
		virtual bool isLoggedIn() const override;
		
		using SearchOptions = Youtube::SearchOptions;
		Promise<YoutubePage<$<MediaItem>>> search(String query, SearchOptions options);
		
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
		virtual LibraryItemGenerator generateLibrary(GenerateLibraryOptions options = GenerateLibraryOptions()) override;
		
		virtual bool canCreatePlaylists() const override;
		virtual ArrayList<Playlist::Privacy> supportedPlaylistPrivacies() const override;
		virtual Promise<$<Playlist>> createPlaylist(String name, CreatePlaylistOptions options) override;
		virtual Promise<bool> isPlaylistEditable($<Playlist> playlist) override;
		
		virtual YoutubePlaybackProvider* player() override;
		virtual const YoutubePlaybackProvider* player() const override;
		
		Track::Data createTrackData(YoutubeVideo video);
		Track::Data createTrackData(YoutubeVideoInfo video);
		Artist::Data createArtistData(YoutubeChannel channel);
		Playlist::Data createPlaylistData(YoutubePlaylist playlist);
		PlaylistItem::Data createPlaylistItemData(YoutubePlaylistItem playlistItem);
		$<MediaItem> createMediaItem(YoutubeSearchResult searchResult);
		
		struct URI {
			String provider;
			String type;
			String id;
		};
		URI parseURI(String uri) const;
		
	private:
		String createURI(String type, String id) const;
		URI parseURL(String url) const;
		
		static MediaItem::Image createImage(YoutubeImage image);
		
		Youtube* youtube;
		YoutubePlaybackProvider* _player;
	};
}

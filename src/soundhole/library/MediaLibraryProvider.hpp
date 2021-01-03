//
//  MediaLibraryProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 5/26/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/MediaProvider.hpp>
#include <soundhole/media/MediaProviderStash.hpp>
#include <soundhole/providers/bandcamp/BandcampMediaProvider.hpp>
#include <soundhole/providers/spotify/SpotifyMediaProvider.hpp>
#include <soundhole/providers/youtube/YoutubeMediaProvider.hpp>

namespace sh {
	class MediaLibraryProvider: public MediaProvider, public MediaProviderStash {
	public:
		MediaLibraryProvider(ArrayList<MediaProvider*> mediaProviders);
		virtual ~MediaLibraryProvider();
		
		virtual String name() const override;
		virtual String displayName() const override;
		
		virtual Promise<bool> login() override;
		virtual void logout() override;
		virtual bool isLoggedIn() const override;
		virtual Promise<ArrayList<String>> getCurrentUserIds() override;
		
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
		virtual Promise<bool> isPlaylistEditable($<Playlist> playlist) override;
		
		virtual MediaPlaybackProvider* player() override;
		virtual const MediaPlaybackProvider* player() const override;
		
		virtual $<MediaItem> createMediaItem(Json json) override;
		ArrayList<MediaProvider*> getMediaProviders();
		void addMediaProvider(MediaProvider*);
		void removeMediaProvider(MediaProvider*);
		virtual MediaProvider* getMediaProvider(const String& name) override;
		template<typename MediaProviderType>
		MediaProviderType* getMediaProvider();
		
	private:
		ArrayList<MediaProvider*> mediaProviders;
	};



	template<typename MediaProviderType>
	MediaProviderType* MediaLibraryProvider::getMediaProvider() {
		for(auto& provider : mediaProviders) {
			if(auto mediaProvider = dynamic_cast<MediaProviderType*>(provider)) {
				return mediaProvider;
			}
		}
		if(auto mediaProvider = dynamic_cast<MediaProviderType*>(this)) {
			return mediaProvider;
		}
		return nullptr;
	}
}

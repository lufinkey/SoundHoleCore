//
//  soundhole.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/library/MediaLibrary.hpp>
#include <soundhole/providers/soundhole/SoundHoleMediaProvider.hpp>
#include <soundhole/providers/bandcamp/BandcampMediaProvider.hpp>
#include <soundhole/providers/spotify/SpotifyMediaProvider.hpp>
#include <soundhole/providers/youtube/YoutubeMediaProvider.hpp>
#include <soundhole/playback/Player.hpp>
#include <soundhole/playback/SystemMediaControls.hpp>
#include <soundhole/utils/Utils.hpp>
#include <soundhole/utils/SoundHoleError.hpp>

namespace sh {
	class SoundHole: public MediaProviderStash {
	public:
		struct Options {
			String dbPath;
			
			Optional<SoundHoleMediaProvider::Options> soundhole;
			Optional<SpotifyMediaProvider::Options> spotify;
			Optional<BandcampMediaProvider::Options> bandcamp;
			Optional<YoutubeMediaProvider::Options> youtube;
		};
		
		SoundHole(Options);
		virtual ~SoundHole();
		
		MediaLibrary* mediaLibrary();
		const MediaLibrary* mediaLibrary() const;
		
		void addMediaProvider(MediaProvider* mediaProvider);
		
		virtual $<MediaItem> parseMediaItem(const Json& json) override;
		virtual MediaProvider* getMediaProvider(const String& name) override;
		template<typename MediaProviderType>
		MediaProviderType* getMediaProvider();
		virtual ArrayList<MediaProvider*> getMediaProviders() override;
		
		bool isSynchronizingLibrary(const String& libraryProviderName);
		bool isSynchronizingLibraries();
		Optional<AsyncQueue::TaskNode> getSynchronizeLibraryTask(const String& libraryProviderName);
		Optional<AsyncQueue::TaskNode> getSynchronizeAllLibrariesTask();
		AsyncQueue::TaskNode synchronizeProviderLibrary(const String& libraryProviderName);
		AsyncQueue::TaskNode synchronizeProviderLibrary(MediaProvider* libraryProvider);
		AsyncQueue::TaskNode synchronizeAllLibraries();
		
		using GetLibraryTracksOptions = MediaLibrary::GetLibraryTracksOptions;
		Promise<$<MediaLibraryTracksCollection>> getLibraryTracksCollection(GetLibraryTracksOptions options = GetLibraryTracksOptions());
		
		using LibraryAlbumsFilters = MediaLibrary::LibraryAlbumsFilters;
		Promise<LinkedList<$<Album>>> getLibraryAlbums(LibraryAlbumsFilters filters = LibraryAlbumsFilters());
		
		using GenerateLibraryAlbumsOptions = MediaLibrary::GenerateLibraryAlbumsOptions;
		using LibraryAlbumsGenerator = MediaLibrary::LibraryAlbumsGenerator;
		LibraryAlbumsGenerator generateLibraryAlbums(GenerateLibraryAlbumsOptions options = GenerateLibraryAlbumsOptions());
		
		using LibraryPlaylistsFilters = MediaLibrary::LibraryPlaylistsFilters;
		Promise<LinkedList<$<Playlist>>> getLibraryPlaylists(LibraryPlaylistsFilters options = LibraryPlaylistsFilters());
		
		using GenerateLibraryPlaylistsOptions = MediaLibrary::GenerateLibraryPlaylistsOptions;
		using LibraryPlaylistsGenerator = MediaLibrary::LibraryPlaylistsGenerator;
		LibraryPlaylistsGenerator generateLibraryPlaylists(GenerateLibraryPlaylistsOptions options = GenerateLibraryPlaylistsOptions());
		
	private:
		MediaLibrary* _mediaLibrary;
		LinkedList<MediaProvider*> _mediaProviders;
	};



	template<typename MediaProviderType>
	MediaProviderType* SoundHole::getMediaProvider() {
		for(auto& provider : mediaProviders) {
			if(auto mediaProvider = dynamic_cast<MediaProviderType*>(provider)) {
				return mediaProvider;
			}
		}
		if(auto mediaProvider = dynamic_cast<MediaProviderType*>(mediaLibrary->proxyProvider())) {
			return mediaProvider;
		}
		return nullptr;
	}
}

//
//  MediaLibrary.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 6/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/database/MediaDatabase.hpp>
#include "collections/librarytracks/MediaLibraryTracksCollection.hpp"
#include "MediaLibraryProvider.hpp"

namespace sh {
	class MediaLibrary {
	public:
		MediaLibrary(const MediaLibrary&) = delete;
		MediaLibrary& operator=(const MediaLibrary&) = delete;
		
		struct Options {
			String dbPath;
			ArrayList<MediaProvider*> mediaProviders;
		};
		MediaLibrary(Options);
		~MediaLibrary();
		
		Promise<void> initialize();
		Promise<void> resetDatabase();
		
		MediaDatabase* database();
		const MediaDatabase* database() const;
		
		MediaProvider* getMediaProvider(String name);
		template<typename MediaProviderType>
		MediaProviderType* getMediaProvider();
		
		bool isSynchronizingLibrary(String libraryProviderName);
		bool isSynchronizingLibraries();
		Optional<AsyncQueue::TaskNode> getSynchronizeLibraryTask(String libraryProviderName);
		Optional<AsyncQueue::TaskNode> getSynchronizeAllLibrariesTask();
		AsyncQueue::TaskNode synchronizeProviderLibrary(String libraryProviderName);
		AsyncQueue::TaskNode synchronizeProviderLibrary(MediaProvider* libraryProvider);
		AsyncQueue::TaskNode synchronizeAllLibraries();
		
		struct GetLibraryTracksFilters {
			MediaProvider* libraryProvider = nullptr;
		};
		String getLibraryTracksCollectionURI(GetLibraryTracksFilters filters = GetLibraryTracksFilters{.libraryProvider=nullptr}) const;
		struct GetLibraryTracksOptions {
			MediaProvider* libraryProvider = nullptr;
			Optional<size_t> offset;
			Optional<size_t> limit;
		};
		Promise<$<MediaLibraryTracksCollection>> getLibraryTracksCollection(GetLibraryTracksOptions options = GetLibraryTracksOptions{.libraryProvider=nullptr});
		
		struct GetLibraryAlbumsFilters {
			MediaProvider* libraryProvider = nullptr;
		};
		Promise<LinkedList<$<Album>>> getLibraryAlbums(GetLibraryAlbumsFilters filters = GetLibraryAlbumsFilters{.libraryProvider=nullptr});
		
		struct GenerateLibraryAlbumsOptions {
			size_t offset = 0;
			size_t chunkSize = 24;
		};
		struct GenerateLibraryAlbumsResult {
			LinkedList<$<Album>> albums;
			size_t offset;
			size_t total;
		};
		using LibraryAlbumsGenerator = ContinuousGenerator<GenerateLibraryAlbumsResult,void>;
		LibraryAlbumsGenerator generateLibraryAlbums(GetLibraryAlbumsFilters filters = GetLibraryAlbumsFilters{.libraryProvider=nullptr}, GenerateLibraryAlbumsOptions options = GenerateLibraryAlbumsOptions{.offset=0,.chunkSize=24});
		
		struct GetLibraryPlaylistsFilters {
			MediaProvider* libraryProvider = nullptr;
		};
		Promise<LinkedList<$<Playlist>>> getLibraryPlaylists(GetLibraryPlaylistsFilters filters = GetLibraryPlaylistsFilters{.libraryProvider=nullptr});
		
		struct GenerateLibraryPlaylistsOptions {
			size_t offset = 0;
			size_t chunkSize = 24;
		};
		struct GenerateLibraryPlaylistsResult {
			LinkedList<$<Playlist>> playlists;
			size_t offset;
			size_t total;
		};
		using LibraryPlaylistsGenerator = ContinuousGenerator<GenerateLibraryPlaylistsResult,void>;
		LibraryPlaylistsGenerator generateLibraryPlaylists(GetLibraryPlaylistsFilters filters = GetLibraryPlaylistsFilters{.libraryProvider=nullptr}, GenerateLibraryPlaylistsOptions options = GenerateLibraryPlaylistsOptions{.offset=0,.chunkSize=24});
		
	private:
		MediaLibraryProvider* libraryProvider;
		MediaDatabase* db;
		AsyncQueue synchronizeQueue;
		AsyncQueue synchronizeAllQueue;
	};



	template<typename MediaProviderType>
	MediaProviderType* MediaLibrary::getMediaProvider() {
		return libraryProvider->getMediaProvider<MediaProviderType>();
	}
}

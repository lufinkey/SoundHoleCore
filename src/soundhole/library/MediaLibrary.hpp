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

namespace sh {
	class MediaLibraryProvider;
	class MediaLibraryTracksCollection;

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
		
		MediaProvider* getMediaProvider(String name);
		
		bool isSynchronizingLibrary(String libraryProviderName);
		bool isSynchronizingLibraries();
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
		
	private:
		MediaLibraryProvider* libraryProvider;
		MediaDatabase* db;
		AsyncQueue synchronizeQueue;
		AsyncQueue synchronizeAllQueue;
	};
}

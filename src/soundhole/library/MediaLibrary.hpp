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
#include "MediaLibraryProxyProvider.hpp"

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
		ArrayList<MediaProvider*> getMediaProviders();
		
		bool isSynchronizingLibrary(String libraryProviderName);
		bool isSynchronizingLibraries();
		Optional<AsyncQueue::TaskNode> getSynchronizeLibraryTask(String libraryProviderName);
		Optional<AsyncQueue::TaskNode> getSynchronizeAllLibrariesTask();
		AsyncQueue::TaskNode synchronizeProviderLibrary(String libraryProviderName);
		AsyncQueue::TaskNode synchronizeProviderLibrary(MediaProvider* libraryProvider);
		AsyncQueue::TaskNode synchronizeAllLibraries();
		
		struct GetLibraryTracksFilters {
			MediaProvider* libraryProvider = nullptr;
			sql::LibraryItemOrderBy orderBy = sql::LibraryItemOrderBy::ADDED_AT;
			sql::Order order = sql::Order::DESC;
		};
		String getLibraryTracksCollectionURI(GetLibraryTracksFilters filters = GetLibraryTracksFilters{
			.libraryProvider=nullptr,
			.orderBy = sql::LibraryItemOrderBy::ADDED_AT,
			.order = sql::Order::DESC
		}) const;
		struct GetLibraryTracksOptions {
			Optional<size_t> offset;
			Optional<size_t> limit;
		};
		Promise<$<MediaLibraryTracksCollection>> getLibraryTracksCollection(GetLibraryTracksFilters filters = GetLibraryTracksFilters{
			.libraryProvider=nullptr,
			.orderBy = sql::LibraryItemOrderBy::ADDED_AT,
			.order = sql::Order::DESC
		}, GetLibraryTracksOptions options = GetLibraryTracksOptions());
		
		struct GetLibraryAlbumsFilters {
			MediaProvider* libraryProvider = nullptr;
			sql::LibraryItemOrderBy orderBy = sql::LibraryItemOrderBy::NAME;
			sql::Order order = sql::Order::ASC;
		};
		Promise<LinkedList<$<Album>>> getLibraryAlbums(GetLibraryAlbumsFilters filters = GetLibraryAlbumsFilters{
			.libraryProvider=nullptr,
			.orderBy = sql::LibraryItemOrderBy::NAME,
			.order = sql::Order::ASC
		});
		
		struct GenerateLibraryAlbumsOptions {
			size_t offset = 0;
			size_t chunkSize = (size_t)-1;
		};
		struct GenerateLibraryAlbumsResult {
			LinkedList<$<Album>> albums;
			size_t offset;
			size_t total;
		};
		using LibraryAlbumsGenerator = ContinuousGenerator<GenerateLibraryAlbumsResult,void>;
		LibraryAlbumsGenerator generateLibraryAlbums(GetLibraryAlbumsFilters filters = GetLibraryAlbumsFilters{
			.libraryProvider=nullptr,
			.orderBy = sql::LibraryItemOrderBy::NAME,
			.order = sql::Order::ASC
		}, GenerateLibraryAlbumsOptions options = GenerateLibraryAlbumsOptions{
			.offset = 0,
			.chunkSize = (size_t)-1
		});
		
		struct GetLibraryPlaylistsFilters {
			MediaProvider* libraryProvider = nullptr;
			sql::LibraryItemOrderBy orderBy = sql::LibraryItemOrderBy::NAME;
			sql::Order order = sql::Order::ASC;
		};
		Promise<LinkedList<$<Playlist>>> getLibraryPlaylists(GetLibraryPlaylistsFilters filters = GetLibraryPlaylistsFilters{
			.libraryProvider=nullptr,
			.orderBy = sql::LibraryItemOrderBy::NAME,
			.order = sql::Order::ASC
		});
		
		struct GenerateLibraryPlaylistsOptions {
			size_t offset = 0;
			size_t chunkSize = (size_t)-1;
		};
		struct GenerateLibraryPlaylistsResult {
			LinkedList<$<Playlist>> playlists;
			size_t offset;
			size_t total;
		};
		using LibraryPlaylistsGenerator = ContinuousGenerator<GenerateLibraryPlaylistsResult,void>;
		LibraryPlaylistsGenerator generateLibraryPlaylists(GetLibraryPlaylistsFilters filters = GetLibraryPlaylistsFilters{
			.libraryProvider=nullptr,
			.orderBy = sql::LibraryItemOrderBy::NAME,
			.order = sql::Order::ASC
		}, GenerateLibraryPlaylistsOptions options = GenerateLibraryPlaylistsOptions{
			.offset=0,
			.chunkSize=24
		});
		
		
		using CreatePlaylistOptions = MediaProvider::CreatePlaylistOptions;
		Promise<$<Playlist>> createPlaylist(String name, MediaProvider* provider, CreatePlaylistOptions options);
		
	private:
		MediaLibraryProxyProvider* libraryProxyProvider;
		MediaDatabase* db;
		AsyncQueue synchronizeQueue;
		AsyncQueue synchronizeAllQueue;
	};



	template<typename MediaProviderType>
	MediaProviderType* MediaLibrary::getMediaProvider() {
		return libraryProxyProvider->getMediaProvider<MediaProviderType>();
	}
}

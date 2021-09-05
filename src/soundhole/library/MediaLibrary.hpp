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
#include "MediaLibraryProxyProvider.hpp"
#include "collections/librarytracks/MediaLibraryTracksCollection.hpp"
#include "collections/playbackhistory/PlaybackHistoryTrackCollection.hpp"

namespace sh {
	class MediaLibrary {
		friend class MediaLibraryProxyProvider;
	public:
		MediaLibrary(const MediaLibrary&) = delete;
		MediaLibrary& operator=(const MediaLibrary&) = delete;
		
		
		struct Options {
			String dbPath;
			MediaProviderStash* mediaProviderStash;
		};
		MediaLibrary(Options);
		~MediaLibrary();
		
		
		Promise<void> initialize();
		Promise<void> resetDatabase();
		
		MediaDatabase* database();
		const MediaDatabase* database() const;
		
		MediaLibraryProxyProvider* proxyProvider();
		const MediaLibraryProxyProvider* proxyProvider() const;
		
		
		bool isSynchronizingLibrary(const String& libraryProviderName);
		bool isSynchronizingLibraries();
		Optional<AsyncQueue::TaskNode> getSynchronizeLibraryTask(const String& libraryProviderName);
		Optional<AsyncQueue::TaskNode> getSynchronizeAllLibrariesTask();
		AsyncQueue::TaskNode synchronizeProviderLibrary(const String& libraryProviderName);
		AsyncQueue::TaskNode synchronizeProviderLibrary(MediaProvider* libraryProvider);
		AsyncQueue::TaskNode synchronizeAllLibraries();
		
		
		using GetLibraryTracksCollectionOptions = MediaLibraryProxyProvider::GetLibraryTracksCollectionOptions;
		Promise<$<MediaLibraryTracksCollection>> getLibraryTracksCollection(GetLibraryTracksCollectionOptions options = GetLibraryTracksCollectionOptions());
		
		
		struct LibraryAlbumsFilters {
			MediaProvider* libraryProvider = nullptr;
			sql::LibraryItemOrderBy orderBy = sql::LibraryItemOrderBy::NAME;
			sql::Order order = sql::Order::ASC;
		};
		Promise<LinkedList<$<Album>>> getLibraryAlbums(LibraryAlbumsFilters filters = LibraryAlbumsFilters{
			.libraryProvider = nullptr,
			.orderBy = sql::LibraryItemOrderBy::NAME,
			.order = sql::Order::ASC
		});
		
		struct GenerateLibraryAlbumsOptions {
			size_t offset = 0;
			size_t chunkSize = 25;
			LibraryAlbumsFilters filters = LibraryAlbumsFilters();
		};
		struct GenerateLibraryAlbumsResult {
			LinkedList<$<Album>> albums;
			size_t offset;
			size_t total;
		};
		using LibraryAlbumsGenerator = ContinuousGenerator<GenerateLibraryAlbumsResult,void>;
		LibraryAlbumsGenerator generateLibraryAlbums(GenerateLibraryAlbumsOptions options = GenerateLibraryAlbumsOptions{
			.offset = 0,
			.chunkSize = 25,
			.filters = {
				.libraryProvider=nullptr,
				.orderBy = sql::LibraryItemOrderBy::NAME,
				.order = sql::Order::ASC
			}
		});
		
		
		struct LibraryPlaylistsFilters {
			MediaProvider* libraryProvider = nullptr;
			sql::LibraryItemOrderBy orderBy = sql::LibraryItemOrderBy::NAME;
			sql::Order order = sql::Order::ASC;
		};
		Promise<LinkedList<$<Playlist>>> getLibraryPlaylists(LibraryPlaylistsFilters filters = LibraryPlaylistsFilters{
			.libraryProvider = nullptr,
			.orderBy = sql::LibraryItemOrderBy::NAME,
			.order = sql::Order::ASC
		});
		
		struct GenerateLibraryPlaylistsOptions {
			size_t offset = 0;
			size_t chunkSize = 25;
			LibraryPlaylistsFilters filters;
		};
		struct GenerateLibraryPlaylistsResult {
			LinkedList<$<Playlist>> playlists;
			size_t offset;
			size_t total;
		};
		using LibraryPlaylistsGenerator = ContinuousGenerator<GenerateLibraryPlaylistsResult,void>;
		LibraryPlaylistsGenerator generateLibraryPlaylists(GenerateLibraryPlaylistsOptions options = GenerateLibraryPlaylistsOptions{
			.offset = 0,
			.chunkSize = 25,
			.filters = {
				.libraryProvider=nullptr,
				.orderBy = sql::LibraryItemOrderBy::NAME,
				.order = sql::Order::ASC
			}
		});
		
		
		using PlaybackHistoryFilters = PlaybackHistoryTrackCollection::Filters;
		struct GetPlaybackHistoryItemsOptions {
			size_t offset = 0;
			size_t limit = 25;
			PlaybackHistoryFilters filters;
		};
		struct GetPlaybackHistoryItemsResult {
			ArrayList<$<PlaybackHistoryItem>> items;
			size_t total;
		};
		Promise<GetPlaybackHistoryItemsResult> getPlaybackHistoryItems(GetPlaybackHistoryItemsOptions options);
		
		struct GeneratePlaybackHistoryItemsOptions {
			size_t offset = 0;
			size_t chunkSize = 25;
			PlaybackHistoryFilters filters;
		};
		using PlaybackHistoryItemGenerator = ContinuousGenerator<ArrayList<$<PlaybackHistoryItem>>,void>;
		PlaybackHistoryItemGenerator generatePlaybackHistoryItems(GeneratePlaybackHistoryItemsOptions options);
		
		
		using GetPlaybackHistoryCollectionOptions = MediaLibraryProxyProvider::GetPlaybackHistoryCollectionOptions;
		Promise<$<PlaybackHistoryTrackCollection>> getPlaybackHistoryCollection(GetPlaybackHistoryCollectionOptions options);
		
		
		using CreatePlaylistOptions = MediaProvider::CreatePlaylistOptions;
		Promise<$<Playlist>> createPlaylist(String name, MediaProvider* provider, CreatePlaylistOptions options);
		
		
		Promise<void> saveTrack($<Track> track);
		Promise<void> unsaveTrack($<Track> track);
		Promise<ArrayList<bool>> hasSavedTracks(ArrayList<$<Track>> tracks);
		
		Promise<void> saveAlbum($<Album> album);
		Promise<void> unsaveAlbum($<Album> album);
		Promise<ArrayList<bool>> hasSavedAlbums(ArrayList<$<Album>> albums);
		
		Promise<void> savePlaylist($<Playlist> playlist);
		Promise<void> unsavePlaylist($<Playlist> playlist);
		Promise<ArrayList<bool>> hasSavedPlaylists(ArrayList<$<Playlist>> playlist);
		
		Promise<void> followArtist($<Artist> artist);
		Promise<void> unfollowArtist($<Artist> artist);
		Promise<ArrayList<bool>> hasFollowedArtists(ArrayList<$<Artist>> artists);
		
		Promise<void> followUserAccount($<UserAccount> user);
		Promise<void> unfollowUserAccount($<UserAccount> user);
		Promise<ArrayList<bool>> hasFollowedUserAccounts(ArrayList<$<UserAccount>> users);
		
	private:
		MediaDatabase* db;
		MediaProviderStash* mediaProviderStash;
		MediaLibraryProxyProvider* libraryProxyProvider;
		AsyncQueue synchronizeQueue;
		AsyncQueue synchronizeAllQueue;
	};
}

//
//  MediaLibraryProxyProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 5/26/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/MediaProvider.hpp>
#include "collections/librarytracks/MediaLibraryTracksCollection.hpp"
#include "collections/playbackhistory/PlaybackHistoryTrackCollection.hpp"

namespace sh {
	class MediaLibrary;


	class MediaLibraryProxyProvider: public MediaProvider {
	public:
		static constexpr auto NAME = "localmedialibrary";
		
		MediaLibraryProxyProvider(MediaLibrary* mediaLibrary);
		virtual ~MediaLibraryProxyProvider();
		
		MediaLibrary* library();
		const MediaLibrary* library() const;
		
		MediaDatabase* database();
		const MediaDatabase* database() const;
		
		virtual String name() const override;
		virtual String displayName() const override;
		
		virtual Promise<bool> login() override;
		virtual void logout() override;
		virtual bool isLoggedIn() const override;
		virtual Promise<ArrayList<String>> getCurrentUserURIs() override;
		
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
		
		virtual MediaPlaybackProvider* player() override;
		virtual const MediaPlaybackProvider* player() const override;
		
		
		using LibraryTracksFilters = MediaLibraryTracksCollection::Filters;
		struct GetLibraryTracksCollectionOptions {
			Optional<size_t> itemsStartIndex;
			Optional<size_t> itemsLimit;
			LibraryTracksFilters filters;
		};
		Promise<$<MediaLibraryTracksCollection>> getLibraryTracksCollection(GetLibraryTracksCollectionOptions options = GetLibraryTracksCollectionOptions());
		
		$<MediaLibraryTracksCollection> libraryTracksCollection(const MediaLibraryTracksCollection::Data& data);
		$<MediaLibraryTracksCollection> libraryTracksCollection(const LibraryTracksFilters& filters, Optional<size_t> itemCount, Map<size_t,MediaLibraryTracksCollectionItem::Data> items);
		
		
		using PlaybackHistoryFilters = PlaybackHistoryTrackCollection::Filters;
		struct GetPlaybackHistoryCollectionOptions {
			Optional<size_t> itemsStartIndex;
			Optional<size_t> itemsLimit;
			PlaybackHistoryFilters filters;
		};
		Promise<$<PlaybackHistoryTrackCollection>> getPlaybackHistoryCollection(GetPlaybackHistoryCollectionOptions options);
		
		$<PlaybackHistoryTrackCollection> playbackHistoryCollection(const PlaybackHistoryTrackCollection::Data& data);
		$<PlaybackHistoryTrackCollection> playbackHistoryCollection(const PlaybackHistoryFilters& filters, Optional<size_t> itemCount, Map<size_t,PlaybackHistoryTrackCollectionItem::Data> items);
		
	private:
		MediaLibrary* _library;
	};
}

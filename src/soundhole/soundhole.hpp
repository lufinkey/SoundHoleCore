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
#include <soundhole/providers/lastfm/LastFMMediaProvider.hpp>
#include <soundhole/playback/Player.hpp>
#include <soundhole/playback/SystemMediaControls.hpp>
#include <soundhole/utils/Utils.hpp>
#include <soundhole/utils/SoundHoleError.hpp>

namespace sh {
	class SoundHole: public MediaProviderStash {
	public:
		struct Options {
			String dbPath;
			Optional<Player::Options> player;
			
			Optional<SoundHoleMediaProvider::Options> soundhole;
			Optional<SpotifyMediaProvider::Options> spotify;
			Optional<BandcampMediaProvider::Options> bandcamp;
			Optional<YoutubeMediaProvider::Options> youtube;
			Optional<LastFMMediaProvider::Options> lastfm;
		};
		
		SoundHole(Options);
		virtual ~SoundHole();
		
		Promise<void> initialize();
		
		MediaLibrary* mediaLibrary();
		const MediaLibrary* mediaLibrary() const;
		
		$<StreamPlayer> streamPlayer();
		$<const StreamPlayer> streamPlayer() const;
		
		$<Player> player();
		$<const Player> player() const;
		
		void addMediaProvider(MediaProvider* mediaProvider);
		
		virtual $<MediaItem> parseMediaItem(const Json& json) override;
		
		virtual MediaProvider* getMediaProvider(const String& name) override;
		template<typename MediaProviderType>
		MediaProviderType* getMediaProvider();
		virtual ArrayList<MediaProvider*> getMediaProviders() override;
		
		ArrayList<Scrobbler*> getScrobblers();
		Scrobbler* getScrobbler(const String& name);
		template<typename ScrobblerType>
		ScrobblerType* getScrobbler();
		
		bool isSynchronizingLibrary(const String& libraryProviderName);
		bool isSynchronizingLibraries();
		Optional<AsyncQueue::TaskNode> getSynchronizeLibraryTask(const String& libraryProviderName);
		Optional<AsyncQueue::TaskNode> getSynchronizeAllLibrariesTask();
		AsyncQueue::TaskNode synchronizeProviderLibrary(const String& libraryProviderName);
		AsyncQueue::TaskNode synchronizeProviderLibrary(MediaProvider* libraryProvider);
		AsyncQueue::TaskNode synchronizeAllLibraries();
		
		using GetLibraryTracksCollectionOptions = MediaLibrary::GetLibraryTracksCollectionOptions;
		Promise<$<MediaLibraryTracksCollection>> getLibraryTracksCollection(GetLibraryTracksCollectionOptions options = GetLibraryTracksCollectionOptions());
		
		using GetPlaybackHistoryItemsOptions = MediaLibrary::GetPlaybackHistoryItemsOptions;
		using GetPlaybackHistoryItemsResult = MediaLibrary::GetPlaybackHistoryItemsResult;
		Promise<GetPlaybackHistoryItemsResult> getPlaybackHistoryItems(GetPlaybackHistoryItemsOptions options);
		
		using GeneratePlaybackHistoryItemsOptions = MediaLibrary::GeneratePlaybackHistoryItemsOptions;
		using PlaybackHistoryItemGenerator = MediaLibrary::PlaybackHistoryItemGenerator;
		PlaybackHistoryItemGenerator generatePlaybackHistoryItems(GeneratePlaybackHistoryItemsOptions options);
		
		using GetPlaybackHistoryCollectionOptions = MediaLibrary::GetPlaybackHistoryCollectionOptions;
		Promise<$<PlaybackHistoryTrackCollection>> getPlaybackHistoryCollection(GetPlaybackHistoryCollectionOptions options);
		
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
		
		
		using CreatePlaylistOptions = MediaLibrary::CreatePlaylistOptions;
		virtual Promise<$<Playlist>> createPlaylist(String name, MediaProvider* provider, CreatePlaylistOptions options);
		
		
	private:
		MediaLibrary* _mediaLibrary;
		LinkedList<MediaProvider*> _mediaProviders;
		LinkedList<Scrobbler*> _scrobblers;
		$<StreamPlayer> _streamPlayer;
		$<Player> _player;
	};



	#pragma mark Soundhole implementation

	template<typename MediaProviderType>
	MediaProviderType* SoundHole::getMediaProvider() {
		for(auto& provider : _mediaProviders) {
			if(auto mediaProvider = dynamic_cast<MediaProviderType*>(provider)) {
				return mediaProvider;
			}
		}
		if(auto mediaProvider = dynamic_cast<MediaProviderType*>(_mediaLibrary->proxyProvider())) {
			return mediaProvider;
		}
		return nullptr;
	}

	template<typename ScrobblerType>
	ScrobblerType* SoundHole::getScrobbler() {
		for(auto& scrobbler : _scrobblers) {
			if(auto typedScrobbler = dynamic_cast<ScrobblerType*>(scrobbler)) {
				return typedScrobbler;
			}
		}
		return nullptr;
	}
}

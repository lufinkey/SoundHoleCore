//
//  SoundHoleMediaProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/27/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/MediaProvider.hpp>
#include <soundhole/storage/StorageProvider.hpp>
#include <soundhole/storage/googledrive/GoogleDriveStorageProvider.hpp>

namespace sh {
	class SoundHoleMediaProvider: public MediaProvider, protected StorageProvider::MediaItemBuilder {
	public:
		static constexpr auto NAME = "soundhole";
		
		struct AuthOptions {
			String sessionPersistKey;
		};
		
		struct Options {
			AuthOptions auth;
			Optional<GoogleDriveStorageProvider::Options> googledrive;
		};
		
		SoundHoleMediaProvider(MediaProviderStash* mediaProviderStash, Options options);
		virtual ~SoundHoleMediaProvider();
		
		virtual String name() const override;
		virtual String displayName() const override;
		
		StorageProvider* primaryStorageProvider();
		const StorageProvider* primaryStorageProvider() const;
		
		StorageProvider* getStorageProvider(const String& name);
		const StorageProvider* getStorageProvider(const String& name) const;
		template<typename StorageProviderType>
		StorageProviderType* getStorageProvider();
		template<typename StorageProviderType>
		const StorageProviderType* getStorageProvider() const;
		
		ArrayList<StorageProvider*> getStorageProviders();
		ArrayList<const StorageProvider*> getStorageProviders() const;
		
		void addStorageProvider(StorageProvider*);
		
		virtual Promise<bool> login() override;
		virtual void logout() override;
		virtual bool isLoggedIn() const override;
		virtual Promise<ArrayList<String>> getCurrentUserURIs() override;
		
		virtual Promise<Track::Data> getTrackData(String uri) override;
		virtual Promise<Artist::Data> getArtistData(String uri) override;
		virtual Promise<Album::Data> getAlbumData(String uri) override;
		virtual Promise<Playlist::Data> getPlaylistData(String uri) override;
		virtual Promise<UserAccount::Data> getUserData(String uri) override;
		
		virtual $<Track> track(const Track::Data& data) override;
		virtual $<Artist> artist(const Artist::Data& data) override;
		virtual $<Album> album(const Album::Data& data) override;
		virtual $<Playlist> playlist(const Playlist::Data& data) override;
		virtual $<UserAccount> userAccount(const UserAccount::Data& data) override;
		
		virtual Promise<ArrayList<$<Track>>> getArtistTopTracks(String artistURI) override;
		virtual ArtistAlbumsGenerator getArtistAlbums(String artistURI) override;
		
		virtual UserPlaylistsGenerator getUserPlaylists(String userURI) override;
		
		virtual Album::MutatorDelegate* createAlbumMutatorDelegate($<Album> album) override;
		virtual Playlist::MutatorDelegate* createPlaylistMutatorDelegate($<Playlist> playlist) override;
		
		virtual bool hasLibrary() const override;
		virtual LibraryItemGenerator generateLibrary(GenerateLibraryOptions options = GenerateLibraryOptions()) override;
		
		virtual bool canFollowArtists() const override;
		virtual Promise<void> followArtist(String artistURI) override;
		virtual Promise<void> followArtist(String artistURI, MediaProvider* provider);
		virtual Promise<void> unfollowArtist(String artistURI) override;
		
		virtual bool canFollowUsers() const override;
		virtual Promise<void> followUser(String userURI) override;
		virtual Promise<void> followUser(String userURI, MediaProvider* provider);
		virtual Promise<void> unfollowUser(String userURI) override;
		
		virtual Promise<void> saveTrack(String trackURI) override;
		virtual Promise<void> unsaveTrack(String trackURI) override;
		virtual Promise<void> saveAlbum(String albumURI) override;
		virtual Promise<void> unsaveAlbum(String albumURI) override;
		
		virtual bool canSavePlaylists() const override;
		virtual Promise<void> savePlaylist(String playlistURI) override;
		virtual Promise<void> savePlaylist(String playlistURI, MediaProvider* provider);
		virtual Promise<void> unsavePlaylist(String playlistURI) override;
		
		virtual bool canCreatePlaylists() const override;
		virtual ArrayList<Playlist::Privacy> supportedPlaylistPrivacies() const override;
		virtual Promise<$<Playlist>> createPlaylist(String name, CreatePlaylistOptions options) override;
		virtual Promise<bool> isPlaylistEditable($<Playlist> playlist) override;
		virtual Promise<void> deletePlaylist(String playlistURI) override;
		
		virtual MediaPlaybackProvider* player() override;
		virtual const MediaPlaybackProvider* player() const override;
		
		struct URI {
			String provider;
			String storageProvider;
			String type;
			String id;
		};
		URI parseURI(String uri) const;
		
	protected:
		virtual StorageProvider::URI parseStorageProviderURI(String uri) const override;
		virtual String createStorageProviderURI(String storageProvider, String type, String id) const override;
		
	private:
		String createURI(String storageProvider, String type, String id) const;
		
		void setPrimaryStorageProvider(StorageProvider*);
		void loadUserPrefs();
		void saveUserPrefs();
		
		MediaProviderStash* mediaProviderStash;
		String sessionPersistKey;
		String primaryStorageProviderName;
		ArrayList<StorageProvider*> storageProviders;
	};



	template<typename StorageProviderType>
	StorageProviderType* SoundHoleMediaProvider::getStorageProvider() {
		for(auto& provider : storageProviders) {
			if(auto storageProvider = dynamic_cast<StorageProviderType*>(provider)) {
				return storageProvider;
			}
		}
		return nullptr;
	}

	template<typename StorageProviderType>
	const StorageProviderType* SoundHoleMediaProvider::getStorageProvider() const {
		for(auto& provider : storageProviders) {
			if(auto storageProvider = dynamic_cast<const StorageProviderType*>(provider)) {
				return storageProvider;
			}
		}
		return nullptr;
	}
}

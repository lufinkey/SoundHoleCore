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
		struct AuthOptions {
			String sessionPersistKey;
		};
		
		struct Options {
			MediaProviderStash* mediaProviderStash = nullptr;
			AuthOptions auth;
			Optional<GoogleDriveStorageProvider::AuthOptions> googledrive;
		};
		
		SoundHoleMediaProvider(Options);
		virtual ~SoundHoleMediaProvider();
		
		virtual String name() const override;
		virtual String displayName() const override;
		
		StorageProvider* primaryStorageProvider();
		const StorageProvider* primaryStorageProvider() const;
		StorageProvider* getStorageProvider(const String& name);
		const StorageProvider* getStorageProvider(const String& name) const;
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
		
		
	private:
		String createURI(String storageProvider, String type, String id) const;
		
		void setPrimaryStorageProvider(StorageProvider*);
		void loadUserPrefs();
		void saveUserPrefs();
		
		String sessionPersistKey;
		String primaryStorageProviderName;
		ArrayList<StorageProvider*> storageProviders;
	};
}

//
//  SoundHoleProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/27/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/MediaProvider.hpp>
#include <soundhole/media/StorageProvider.hpp>
#include <soundhole/storage/googledrive/GoogleDriveStorageProvider.hpp>

namespace sh {
	class SoundHoleProvider: public MediaProvider {
	public:
		struct Options {
			Optional<GoogleDriveStorageProvider::Options> googledrive;
		};
		
		SoundHoleProvider(Options);
		virtual ~SoundHoleProvider();
		
		virtual String name() const override;
		virtual String displayName() const override;
		
		StorageProvider* primaryStorageProvider();
		const StorageProvider* primaryStorageProvider() const;
		StorageProvider* getStorageProvider(const String& name);
		const StorageProvider* getStorageProvider(const String& name) const;
		
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
		
		struct UserID {
			String storageProvider;
			String id;
		};
		UserID parseUserID(String userId) const;
		
	private:
		String createURI(String storageProvider, String type, String id) const;
		String createUserID(String storageProvider, String id) const;
		
		String primaryStorageProviderName;
		ArrayList<StorageProvider*> storageProviders;
	};
}

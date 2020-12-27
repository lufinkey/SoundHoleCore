//
//  GoogleDriveStorageProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/26/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/storage/StorageProvider.hpp>
#include <soundhole/utils/js/JSWrapClass.hpp>

namespace sh {
	class GoogleDriveStorageProvider: public StorageProvider, private JSWrapClass {
	public:
		struct Options {
			String clientId;
			String clientSecret;
			String redirectURL;
			String sessionPersistKey;
		};
		
		GoogleDriveStorageProvider(Options options);
		virtual ~GoogleDriveStorageProvider();
		
		virtual String name() const override;
		virtual String displayName() const override;
		
		virtual bool canStorePlaylists() const override;
		virtual Promise<Playlist> createPlaylist(String name, CreatePlaylistOptions options = CreatePlaylistOptions()) override;
		virtual Promise<Playlist> getPlaylist(String id) override;
		virtual Promise<void> deletePlaylist(String id) override;
		
	private:
		virtual void initializeJS(napi_env env) override;
		
		napi_ref jsRef;
		Options options;
	};
}

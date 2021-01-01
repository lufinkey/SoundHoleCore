//
//  StorageProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/26/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class StorageProvider {
	public:
		virtual ~StorageProvider() {}
		
		struct Playlist {
			String id;
			String name;
			String description;
			
			static Playlist fromJson(Json);
			#ifdef NODE_API_MODULE
			static Playlist fromNapiObject(Napi::Object);
			#endif
		};
		
		virtual String name() const = 0;
		virtual String displayName() const = 0;
		
		virtual Promise<bool> login() = 0;
		virtual void logout() = 0;
		virtual bool isLoggedIn() const = 0;
		
		virtual bool canStorePlaylists() const = 0;
		struct CreatePlaylistOptions {
			String description;
		};
		virtual Promise<Playlist> createPlaylist(String name, CreatePlaylistOptions options = CreatePlaylistOptions()) = 0;
		virtual Promise<Playlist> getPlaylist(String id) = 0;
		virtual Promise<void> deletePlaylist(String id) = 0;
	};
}

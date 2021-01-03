//
//  StorageProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/26/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "NamedProvider.hpp"
#include "AuthedProvider.hpp"

namespace sh {
	class StorageProvider: public NamedProvider, public AuthedProvider {
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
		
		virtual bool canStorePlaylists() const = 0;
		struct CreatePlaylistOptions {
			String description;
		};
		virtual Promise<Playlist> createPlaylist(String name, CreatePlaylistOptions options = CreatePlaylistOptions()) = 0;
		virtual Promise<Playlist> getPlaylist(String id) = 0;
		virtual Promise<void> deletePlaylist(String id) = 0;
	};
}

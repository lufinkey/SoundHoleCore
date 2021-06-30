//
//  StorageProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/26/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/NamedProvider.hpp>
#include <soundhole/media/AuthedProvider.hpp>
#include <soundhole/media/MediaProvider.hpp>
#include <soundhole/media/Track.hpp>
#include <soundhole/media/Album.hpp>
#include <soundhole/media/Artist.hpp>
#include <soundhole/media/Playlist.hpp>

namespace sh {
	class StorageProvider: public NamedProvider, public AuthedProvider {
	public:
		using UserPlaylistsGenerator = MediaProvider::UserPlaylistsGenerator;
		static constexpr auto BASE_FOLDER_NAME = "My SoundHole";
		static constexpr auto APP_KEY = "SoundHole";
		
		struct URI {
			String storageProvider;
			String type;
			String id;
			
			Json toJson() const;
			static URI fromJson(const Json&);
			
			#ifdef NODE_API_MODULE
			Napi::Object toNapiObject(napi_env env) const;
			static URI fromNapiObject(Napi::Object);
			#endif
		};
		
		class MediaItemBuilder {
		public:
			virtual String name() const = 0;
			
			virtual String createStorageProviderURI(String storageProvider, String type, String id) const = 0;
			virtual URI parseStorageProviderURI(String uri) const = 0;
			
			virtual $<Track> track(const Track::Data& data) = 0;
			virtual $<Artist> artist(const Artist::Data& data) = 0;
			virtual $<Album> album(const Album::Data& data) = 0;
			virtual $<Playlist> playlist(const Playlist::Data& data) = 0;
			virtual $<UserAccount> userAccount(const UserAccount::Data& data) = 0;
		};
		
		virtual ~StorageProvider() {}
		
		virtual bool canStorePlaylists() const = 0;
		using CreatePlaylistOptions = MediaProvider::CreatePlaylistOptions;
		virtual Promise<$<Playlist>> createPlaylist(String name, CreatePlaylistOptions options = CreatePlaylistOptions()) = 0;
		virtual Promise<Playlist::Data> getPlaylistData(String uri) = 0;
		virtual Promise<void> deletePlaylist(String uri) = 0;
		virtual Promise<bool> isPlaylistEditable($<Playlist> playlist) = 0;
		
		virtual UserPlaylistsGenerator getUserPlaylists(String userURI) = 0;
		virtual UserPlaylistsGenerator getMyPlaylists() = 0;
		
		virtual Playlist::MutatorDelegate* createPlaylistMutatorDelegate($<Playlist> playlist) = 0;
	};
}

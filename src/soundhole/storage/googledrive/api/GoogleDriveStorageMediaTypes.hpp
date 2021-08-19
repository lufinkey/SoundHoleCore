//
//  GoogleDriveStorageMediaTypes.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/23/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/Playlist.hpp>
#include <soundhole/media/MediaProviderStash.hpp>

namespace sh {
	struct GoogleDriveStorageProviderUser {
		String uri;
		String kind;
		String displayName;
		String photoLink;
		String permissionId;
		String emailAddress;
		bool me;
		String baseFolderId;
		
		Json toJson() const;
		
		static GoogleDriveStorageProviderUser fromJson(const Json&);
		#ifdef NODE_API_MODULE
		static GoogleDriveStorageProviderUser fromNapiObject(Napi::Object);
		#endif
	};


	struct GoogleDrivePlaylistItemsPage {
		size_t offset;
		size_t total;
		ArrayList<PlaylistItem::Data> items;
		
		static GoogleDrivePlaylistItemsPage fromJson(const Json& json, MediaProviderStash* stash);
	};


	template<typename T>
	struct GoogleDriveFilesPage {
		ArrayList<T> items;
		String nextPageToken;
		
		static GoogleDriveFilesPage<T> fromJson(const Json& json);
		static GoogleDriveFilesPage<T> fromJson(const Json& json, MediaProviderStash* stash);
	};


	template<typename T>
	struct GoogleSheetDBPage {
		size_t offset;
		size_t total;
		ArrayList<T> items;
		
		#ifdef NODE_API_MODULE
		static GoogleSheetDBPage<T> fromNapiObject(Napi::Object);
		#endif
	};
}

#include "GoogleDriveStorageMediaTypes.impl.hpp"

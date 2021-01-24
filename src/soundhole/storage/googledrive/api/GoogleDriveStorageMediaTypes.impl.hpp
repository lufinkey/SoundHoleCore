//
//  GoogleDriveStorageMediaTypes.impl.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/24/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/utils/js/JSUtils.hpp>

namespace sh {
	template<typename T>
	GoogleDriveFilesPage<T> GoogleDriveFilesPage<T>::fromJson(const Json& json) {
		return GoogleDriveFilesPage<T>{
			.items = jsutils::arrayListFromJson<T>(json["items"], [](auto& item) -> T {
				return T::fromJson(item);
			}),
			.nextPageToken = json["nextPageToken"].string_value()
		};
	}

	template<typename T>
	GoogleDriveFilesPage<T> GoogleDriveFilesPage<T>::fromJson(const Json& json, MediaProviderStash* stash) {
		return GoogleDriveFilesPage<T>{
			.items = jsutils::arrayListFromJson<T>(json["items"], [&](auto& item) -> T {
				return T::fromJson(item, stash);
			}),
			.nextPageToken = json["nextPageToken"].string_value()
		};
	}
}

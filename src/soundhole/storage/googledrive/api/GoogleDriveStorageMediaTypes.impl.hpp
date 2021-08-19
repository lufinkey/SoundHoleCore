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
			.items = jsutils::arrayListFromJson(json["items"], [](auto& item) -> T {
				return T::fromJson(item);
			}),
			.nextPageToken = json["nextPageToken"].string_value()
		};
	}

	template<typename T>
	GoogleDriveFilesPage<T> GoogleDriveFilesPage<T>::fromJson(const Json& json, MediaProviderStash* stash) {
		return GoogleDriveFilesPage<T>{
			.items = jsutils::arrayListFromJson(json["items"], [&](auto& item) -> T {
				return T::fromJson(item, stash);
			}),
			.nextPageToken = json["nextPageToken"].string_value()
		};
	}

	#ifdef NODE_API_MODULE
	template<typename T>
	GoogleSheetDBPage<T> GoogleSheetDBPage<T>::fromNapiObject(Napi::Object obj) {
		return GoogleSheetDBPage<T>{
			.offset = jsutils::nonNullSizePropFromNapiObject(obj, "offset"),
			.total = jsutils::nonNullSizePropFromNapiObject(obj, "total"),
			.items = jsutils::arrayListFromNapiValue(obj.Get("items"), [](Napi::Value item) {
				return T::fromNapiObject(item.As<Napi::Object>());
			})
		};
	}
	#endif
}

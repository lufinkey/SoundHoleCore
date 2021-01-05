//
//  StorageProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/26/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include <napi.h>
#include "StorageProvider.hpp"
#include <soundhole/utils/js/JSUtils.hpp>

namespace sh {
	StorageProvider::User StorageProvider::User::fromJson(const Json& json) {
		return User{
			.id = json["id"].string_value(),
			.name = json["name"].string_value(),
			.imageURL = json["imageURL"].string_value()
		};
	}

	#ifdef NODE_API_MODULE
	StorageProvider::User StorageProvider::User::fromNapiObject(Napi::Object obj) {
		return User{
			.id = jsutils::nonNullStringPropFromNapiObject(obj, "id"),
			.name = jsutils::nonNullStringPropFromNapiObject(obj, "name"),
			.imageURL = jsutils::nonNullStringPropFromNapiObject(obj, "imageURL")
		};
	}
	#endif

	StorageProvider::Playlist StorageProvider::Playlist::fromJson(const Json& json) {
		return Playlist{
			.id = json["id"].string_value(),
			.name = json["name"].string_value(),
			.description = json["description"].string_value()
		};
	}

	#ifdef NODE_API_MODULE
	StorageProvider::Playlist StorageProvider::Playlist::fromNapiObject(Napi::Object obj) {
		auto owner = obj.Get("owner").As<Napi::Object>();
		return Playlist{
			.id = jsutils::nonNullStringPropFromNapiObject(obj, "id"),
			.name = jsutils::nonNullStringPropFromNapiObject(obj, "name"),
			.versionId = jsutils::stringFromNapiValue(obj.Get("versionId")),
			.description = jsutils::stringFromNapiValue(obj.Get("description")),
			.privacy = jsutils::stringFromNapiValue(obj.Get("privacy")),
			.owner = (owner.IsEmpty() || owner.IsNull() || owner.IsUndefined()) ? std::nullopt : maybe(User::fromNapiObject(owner))
		};
	}
	#endif
}

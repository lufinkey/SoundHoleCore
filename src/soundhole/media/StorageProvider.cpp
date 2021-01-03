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
	StorageProvider::Playlist StorageProvider::Playlist::fromJson(Json json) {
		return Playlist{
			.id = json["id"].string_value(),
			.name = json["name"].string_value(),
			.description = json["description"].string_value()
		};
	}

	#ifdef NODE_API_MODULE
	StorageProvider::Playlist StorageProvider::Playlist::fromNapiObject(Napi::Object obj) {
		return Playlist{
			.id = jsutils::nonNullStringPropFromNapiObject(obj, "id"),
			.name = jsutils::nonNullStringPropFromNapiObject(obj, "name"),
			.description = jsutils::nonNullStringPropFromNapiObject(obj, "description")
		};
	}
	#endif
}

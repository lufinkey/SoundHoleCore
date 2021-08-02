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
	Json StorageProvider::URI::toJson() const {
		return Json::object{
			{ "storageProvider", (std::string)storageProvider },
			{ "type", (std::string)type },
			{ "id", (std::string)id }
		};
	}

	StorageProvider::URI StorageProvider::URI::fromJson(const Json& json) {
		return URI{
			.storageProvider = json["storageProvider"].string_value(),
			.type = json["type"].string_value(),
			.id = json["id"].string_value()
		};
	}

	#ifdef NODE_API_MODULE

	Napi::Object StorageProvider::URI::toNapiObject(napi_env env) const {
		auto uri = Napi::Object::New(env);
		uri.Set("storageProvider", Napi::String::New(env, storageProvider));
		uri.Set("type", Napi::String::New(env, type));
		uri.Set("id", Napi::String::New(env, id));
		return uri;
	}

	StorageProvider::URI StorageProvider::URI::fromNapiObject(Napi::Object obj) {
		return URI{
			.storageProvider = obj.Get("storageProvider").As<Napi::String>().Utf8Value(),
			.type = obj.Get("type").As<Napi::String>().Utf8Value(),
			.id = obj.Get("id").ToString().Utf8Value()
		};
	}

	Napi::Object StorageProvider::NewFollowedItem::toNapiObject(napi_env env) const {
		auto obj = Napi::Object::New(env);
		obj.Set("uri", uri.toNodeJSValue(env));
		obj.Set("provider", provider.toNodeJSValue(env));
		return obj;
	}

	#endif
}

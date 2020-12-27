//
//  GoogleDriveStorageProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/26/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include <napi.h>
#include "GoogleDriveStorageProvider.hpp"
#include <soundhole/scripts/Scripts.hpp>
#include <soundhole/utils/js/JSWrapClass.impl.hpp>

namespace sh {
	GoogleDriveStorageProvider::GoogleDriveStorageProvider()
	: jsRef(nullptr) {
		//
	}

	GoogleDriveStorageProvider::~GoogleDriveStorageProvider() {
		auto jsRef = this->jsRef;
		if(jsRef != nullptr) {
			queueJSDestruct([=](napi_env env) {
				auto napiRef = Napi::ObjectReference(env, jsRef);
				if(napiRef.Unref() == 0) {
					napiRef.Reset();
				} else {
					napiRef.SuppressDestruct();
				}
			});
		}
	}



	void GoogleDriveStorageProvider::initializeJS(napi_env env) {
		if(this->jsRef == nullptr) {
			// create options
			auto gdOptions = Napi::Object::New(env);
			if(!options.clientId.empty()) {
				gdOptions.Set("clientId", Napi::String::New(env, options.clientId));
			}
			if(!options.clientSecret.empty()) {
				gdOptions.Set("clientSecret", Napi::String::New(env, options.clientSecret));
			}
			
			// instantiate bandcamp
			auto jsExports = scripts::getJSExports(env);
			auto GoogleDriveClass = jsExports.Get("GoogleDriveStorageProvider").As<Napi::Function>();
			auto gDrive = GoogleDriveClass.New({ gdOptions });
			auto gDriveRef = Napi::ObjectReference::New(gDrive, 1);
			gDriveRef.SuppressDestruct();
			jsRef = gDriveRef;
		}
	}



	String GoogleDriveStorageProvider::name() const {
		return "googledrive";
	}

	String GoogleDriveStorageProvider::displayName() const {
		return "Google Drive";
	}



	Promise<StorageProvider::Playlist> GoogleDriveStorageProvider::createPlaylist(String name, CreatePlaylistOptions options) {
		return performAsyncFunc<StorageProvider::Playlist>(jsRef, "createPlaylist", [=](napi_env env) {
			auto optionsObj = Napi::Object::New(env);
			if(!options.description.empty()) {
				optionsObj.Set("description", Napi::String::New(env, options.description));
			}
			return std::vector<napi_value>{
				Napi::String::New(env, name),
				optionsObj
			};
		}, [](napi_env env, Napi::Value value) {
			return StorageProvider::Playlist::fromNapiObject(value.As<Napi::Object>());
		});
	}

	Promise<StorageProvider::Playlist> GoogleDriveStorageProvider::getPlaylist(String id) {
		return performAsyncFunc<StorageProvider::Playlist>(jsRef, "getPlaylist", [=](napi_env env) {
			return std::vector<napi_value>{
				Napi::String::New(env, id)
			};
		}, [](napi_env env, Napi::Value value) {
			return StorageProvider::Playlist::fromNapiObject(value.As<Napi::Object>());
		});
	}

	Promise<void> GoogleDriveStorageProvider::deletePlaylist(String id) {
		return performAsyncFunc<void>(jsRef, "deletePlaylist", [=](napi_env env) {
			return std::vector<napi_value>{
				Napi::String::New(env, id)
			};
		}, nullptr);
	}
}

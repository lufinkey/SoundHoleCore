//
//  GoogleDriveStorageProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/26/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include <napi.h>
#include "GoogleDriveStorageProvider.hpp"
#include "api/GoogleDriveMediaTypes.hpp"
#include <soundhole/scripts/Scripts.hpp>
#include <soundhole/utils/js/JSWrapClass.impl.hpp>
#include <soundhole/utils/HttpClient.hpp>

namespace sh {
	GoogleDriveStorageProvider::GoogleDriveStorageProvider(Options options)
	: jsRef(nullptr), options(options) {
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
			if(!options.redirectURL.empty()) {
				gdOptions.Set("redirectURL", Napi::String::New(env, options.redirectURL));
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

	#ifdef NODE_API_MODULE
	template<typename Result>
	Promise<Result> GoogleDriveStorageProvider::performAsyncJSAPIFunc(String funcName, Function<std::vector<napi_value>(napi_env)> createArgs, Function<Result(napi_env,Napi::Value)> mapper) {
		return performAsyncFunc<Result>(jsRef, funcName, createArgs, mapper, {
			.beforeFuncCall=[=](napi_env env) {
				this->updateSessionFromJS(env);
			},
			.afterFuncFinish=[=](napi_env env) {
				this->updateSessionFromJS(env);
			}
		});
	}
	#endif



	String GoogleDriveStorageProvider::name() const {
		return "googledrive";
	}

	String GoogleDriveStorageProvider::displayName() const {
		return "Google Drive";
	}



	#pragma mark Authentication

	String GoogleDriveStorageProvider::getWebAuthenticationURL(String codeChallenge) const {
		auto query = std::map<String,String>{
			{ "client_id", options.clientId },
			{ "redirect_uri", options.redirectURL },
			{ "response_type", "code" },
			{ "scope", "https://www.googleapis.com/auth/drive" }
		};
		if(!codeChallenge.empty()) {
			query.insert_or_assign("code_challenge", codeChallenge);
			query.insert_or_assign("code_challenge_method", "plain");
		}
		return "https://accounts.google.com/o/oauth2/v2/auth?" + utils::makeQueryString(query);
	}



	Json GoogleDriveStorageProvider::sessionFromJS(napi_env env) {
		// get api objects
		auto jsApi = jsutils::jsValue<Napi::Object>(env, jsRef);
		if(jsApi.IsEmpty()) {
			throw std::logic_error("js object not initialized");
		}
		auto json_encode = scripts::getJSExports(env).Get("json_encode").As<Napi::Function>();
		// get credentials object
		auto credentialsObj = jsApi.Get("session");
		auto credentialsJsonStr = json_encode.Call({ credentialsObj }).As<Napi::String>().Utf8Value();
		std::string parseError;
		auto credentials = Json::parse(credentialsJsonStr, parseError);
		if(!parseError.empty()) {
			return Json();
		}
		return credentials;
	}

	void GoogleDriveStorageProvider::updateSessionFromJS(napi_env env) {
		bool wasLoggedIn = isLoggedIn();
		this->credentials = sessionFromJS(env);
		bool loggedIn = isLoggedIn();
		if(loggedIn != wasLoggedIn) {
			// TODO refresh current user
		}
	}

	Promise<void> GoogleDriveStorageProvider::handleOAuthRedirect(std::map<String,String> params, String codeVerifier) {
		return performAsyncFunc<void>(jsRef, "loginWithRedirectParams", [=](napi_env env) {
			auto paramsObj = Napi::Object::New(env);
			for(auto& pair : params) {
				paramsObj.Set(pair.first, Napi::String::New(env, pair.second));
			}
			auto optionsObj = Napi::Object::New(env);
			if(!codeVerifier.empty()) {
				optionsObj.Set("codeVerifier", Napi::String::New(env, codeVerifier));
			}
			if(!this->options.clientId.empty()) {
				optionsObj.Set("clientId", Napi::String::New(env, this->options.clientId));
			}
			return std::vector<napi_value>{
				paramsObj,
				optionsObj
			};
		}, [=](napi_env env, Napi::Value value) {
			auto credentials = sessionFromJS(env);
			if(credentials.is_null() || !credentials.is_object()) {
				throw std::runtime_error("Failed to login with authorization code");
			}
			this->credentials = credentials;
		});
	}



	void GoogleDriveStorageProvider::logout() {
		queueJS([=](napi_env env) {
			// get api object and make call
			auto jsApi = jsutils::jsValue<Napi::Object>(env, jsRef);
			if(jsApi.IsEmpty()) {
				return;
			}
			// logout
			jsApi.Get("logout").As<Napi::Function>().Call(jsApi, {});
		});
		credentials = Json();
	}

	bool GoogleDriveStorageProvider::isLoggedIn() const {
		return !credentials.is_null();
	}

	Promise<ArrayList<String>> GoogleDriveStorageProvider::getCurrentUserIds() {
		return performAsyncJSAPIFunc<ArrayList<String>>("getCurrentUser", [=](napi_env env) {
			return std::vector<napi_value>{};
		}, [](napi_env env, Napi::Value value) {
			auto user = GoogleDriveUser::fromNapiObject(value.As<Napi::Object>());
			return ArrayList<String>{ user.emailAddress };
		});
	}



	#pragma mark Playlists

	Promise<StorageProvider::Playlist> GoogleDriveStorageProvider::createPlaylist(String name, CreatePlaylistOptions options) {
		return performAsyncJSAPIFunc<StorageProvider::Playlist>("createPlaylist", [=](napi_env env) {
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
		return performAsyncJSAPIFunc<StorageProvider::Playlist>("getPlaylist", [=](napi_env env) {
			return std::vector<napi_value>{
				Napi::String::New(env, id)
			};
		}, [](napi_env env, Napi::Value value) {
			return StorageProvider::Playlist::fromNapiObject(value.As<Napi::Object>());
		});
	}

	Promise<void> GoogleDriveStorageProvider::deletePlaylist(String id) {
		return performAsyncJSAPIFunc<void>("deletePlaylist", [=](napi_env env) {
			return std::vector<napi_value>{
				Napi::String::New(env, id)
			};
		}, nullptr);
	}
}

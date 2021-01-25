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
#include <soundhole/utils/HttpClient.hpp>
#include <soundhole/utils/Utils.hpp>
#include "mutators/GoogleDrivePlaylistMutatorDelegate.hpp"

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
			if(!options.auth.clientId.empty()) {
				gdOptions.Set("clientId", Napi::String::New(env, options.auth.clientId));
			}
			if(!options.auth.clientSecret.empty()) {
				gdOptions.Set("clientSecret", Napi::String::New(env, options.auth.clientSecret));
			}
			if(!options.auth.redirectURL.empty()) {
				gdOptions.Set("redirectURL", Napi::String::New(env, options.auth.redirectURL));
			}
			
			// create media item builder
			if(options.mediaItemBuilder != nullptr) {
				Napi::Object mediaItemBuilder;
				// createStorageProviderURI
				mediaItemBuilder.Set("createStorageProviderURI", Napi::Function::New(env, [=](const Napi::CallbackInfo& info) -> Napi::Value {
					napi_env env = info.Env();
					if(info.Length() != 3) {
						throw std::invalid_argument(std::to_string(info.Length())+" is not a valid number of arguments for createStorageProviderURI");
					}
					auto storageProvider = info[0].As<Napi::String>();
					auto type = info[1].As<Napi::String>();
					auto id = info[2].As<Napi::String>();
					if(!storageProvider.IsString()) {
						throw std::invalid_argument("storageProvider argument must be a string");
					} else if(!type.IsString()) {
						throw std::invalid_argument("type argument must be a string");
					} else if(!id.IsString() && !id.IsNumber()) {
						throw std::invalid_argument("id argument must be a string");
					}
					auto uri = this->options.mediaItemBuilder->createStorageProviderURI(storageProvider.Utf8Value(), type.Utf8Value(), id.ToString().Utf8Value());
					return Napi::String::New(env, uri);
				}));
				// parseStorageProviderURI
				mediaItemBuilder.Set("parseStorageProviderURI", Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
					napi_env env = info.Env();
					if(info.Length() != 1) {
						throw std::invalid_argument(std::to_string(info.Length())+" is not a valid number of arguments for parseStorageProviderURI");
					}
					auto uri = info[0].As<Napi::String>();
					if(!uri.IsString()) {
						throw std::invalid_argument("uri argument must be a string");
					}
					auto uriParts = this->options.mediaItemBuilder->parseStorageProviderURI(uri.Utf8Value());
					return uriParts.toNapiObject(env);
				}));
				gdOptions.Set("mediaItemBuilder", mediaItemBuilder);
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
			{ "client_id", options.auth.clientId },
			{ "redirect_uri", options.auth.redirectURL },
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
		// get session object
		auto sessionObj = jsApi.Get("session");
		auto sessionJsonStr = json_encode.Call({ sessionObj }).As<Napi::String>().Utf8Value();
		std::string parseError;
		auto session = Json::parse(sessionJsonStr, parseError);
		if(!parseError.empty()) {
			return Json();
		}
		return session;
	}

	void GoogleDriveStorageProvider::updateSessionFromJS(napi_env env) {
		bool wasLoggedIn = isLoggedIn();
		this->session = sessionFromJS(env);
		bool loggedIn = isLoggedIn();
		// update identity if login state has changed
		if(loggedIn != wasLoggedIn) {
			if(loggedIn) {
				this->setIdentityNeedsRefresh();
			} else {
				this->storeIdentity(std::nullopt);
			}
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
			if(!this->options.auth.clientId.empty()) {
				optionsObj.Set("clientId", Napi::String::New(env, this->options.auth.clientId));
			}
			return std::vector<napi_value>{
				paramsObj,
				optionsObj
			};
		}, [=](napi_env env, Napi::Value value) {
			auto session = sessionFromJS(env);
			if(session.is_null() || !session.is_object()) {
				throw std::runtime_error("Failed to login with authorization code");
			}
			this->session = session;
			// re-fetch identity
			this->storeIdentity(std::nullopt);
			this->setIdentityNeedsRefresh();
			this->getIdentity();
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
		session = Json();
		this->storeIdentity(std::nullopt);
	}

	bool GoogleDriveStorageProvider::isLoggedIn() const {
		return !session.is_null();
	}



	#pragma mark Current User

	Promise<ArrayList<String>> GoogleDriveStorageProvider::getCurrentUserURIs() {
		return getIdentity().map<ArrayList<String>>([=](Optional<GoogleDriveStorageProviderUser> user) {
			if(!user) {
				return ArrayList<String>{};
			}
			return ArrayList<String>{ user->uri };
		});
	}

	String GoogleDriveStorageProvider::getIdentityFilePath() const {
		if(options.auth.sessionPersistKey.empty()) {
			return String();
		}
		return utils::getCacheDirectoryPath()+"/"+name()+"_identity_"+options.auth.sessionPersistKey+".json";
	}

	Promise<Optional<GoogleDriveStorageProviderUser>> GoogleDriveStorageProvider::fetchIdentity() {
		if(!isLoggedIn()) {
			return Promise<Optional<GoogleDriveStorageProviderUser>>::resolve(std::nullopt);
		}
		return performAsyncJSAPIFunc<Optional<GoogleDriveStorageProviderUser>>("getCurrentUser", [=](napi_env env) {
			return std::vector<napi_value>{};
		}, [](napi_env env, Napi::Value value) {
			return GoogleDriveStorageProviderUser::fromNapiObject(value.As<Napi::Object>());
		});
	}



	#pragma mark Playlists

	bool GoogleDriveStorageProvider::canStorePlaylists() const {
		return true;
	}

	Promise<$<Playlist>> GoogleDriveStorageProvider::createPlaylist(String name, CreatePlaylistOptions options) {
		return performAsyncJSAPIFunc<$<Playlist>>("createPlaylist", [=](napi_env env) {
			return std::vector<napi_value>{
				Napi::String::New(env, name),
				options.toNapiObject(env)
			};
		}, [=](napi_env env, Napi::Value value) {
			auto jsExports = scripts::getJSExports(env);
			auto json_encode = jsExports.Get("json_encode").As<Napi::Function>();
			auto jsonString = json_encode.Call({ value }).As<Napi::String>().Utf8Value();
			std::string jsonError;
			auto json = Json::parse(jsonString, jsonError);
			if(!jsonError.empty()) {
				throw std::runtime_error("failed to decode json for createPlaylist result");
			}
			return this->options.mediaItemBuilder->playlist(
				Playlist::Data::fromJson(json, this->options.mediaProviderStash));
		});
	}

	Promise<Playlist::Data> GoogleDriveStorageProvider::getPlaylistData(String uri) {
		return performAsyncJSAPIFunc<Playlist::Data>("getPlaylist", [=](napi_env env) {
			return std::vector<napi_value>{
				Napi::String::New(env, uri)
			};
		}, [=](napi_env env, Napi::Value value) {
			auto jsExports = scripts::getJSExports(env);
			auto json_encode = jsExports.Get("json_encode").As<Napi::Function>();
			auto jsonString = json_encode.Call({ value }).As<Napi::String>().Utf8Value();
			std::string jsonError;
			auto json = Json::parse(jsonString, jsonError);
			if(!jsonError.empty()) {
				throw std::runtime_error("failed to decode json for createPlaylist result");
			}
			return Playlist::Data::fromJson(json, this->options.mediaProviderStash);
		});
	}

	Promise<void> GoogleDriveStorageProvider::deletePlaylist(String uri) {
		return performAsyncJSAPIFunc<void>("deletePlaylist", [=](napi_env env) {
			return std::vector<napi_value>{
				Napi::String::New(env, uri)
			};
		}, nullptr);
	}

	Promise<bool> GoogleDriveStorageProvider::isPlaylistEditable($<Playlist> playlist) {
		auto uri = playlist->uri();
		return performAsyncJSAPIFunc<bool>("isPlaylistEditable", [=](napi_env env) {
			return std::vector<napi_value>{
				Napi::String::New(env, uri)
			};
		}, [=](napi_env env, Napi::Value value) {
			return value.As<Napi::Boolean>().Value();
		});
	}


	GoogleDriveStorageProvider::UserPlaylistsGenerator GoogleDriveStorageProvider::getUserPlaylists(String userURI) {
		struct SharedData {
			String pageToken;
		};
		auto sharedData = fgl::new$<SharedData>();
		using YieldResult = typename UserPlaylistsGenerator::YieldResult;
		return UserPlaylistsGenerator([=]() {
			return performAsyncJSAPIFunc<YieldResult>("getUserPlaylists", [=](napi_env env) {
				auto optionsObj = Napi::Object::New(env);
				if(!sharedData->pageToken.empty()) {
					optionsObj.Set("pageToken", Napi::String::New(env, sharedData->pageToken));
				}
				return std::vector<napi_value>{
					Napi::String::New(env, userURI),
					optionsObj
				};
			}, [=](napi_env env, Napi::Value value) {
				auto jsExports = scripts::getJSExports(env);
				auto json_encode = jsExports.Get("json_encode").As<Napi::Function>();
				auto jsonString = json_encode.Call({ value }).As<Napi::String>().Utf8Value();
				std::string jsonError;
				auto json = Json::parse(jsonString, jsonError);
				if(!jsonError.empty()) {
					throw std::runtime_error("failed to decode json for createPlaylist result");
				}
				auto page = GoogleDriveFilesPage<Playlist::Data>::fromJson(json, this->options.mediaProviderStash);
				auto loadBatch = MediaProvider::LoadBatch<$<Playlist>>{
					.items = page.items.template map<$<Playlist>>([=](auto& playlistData) {
						return this->options.mediaItemBuilder->playlist(playlistData);
					}),
					.total = std::nullopt
				};
				sharedData->pageToken = page.nextPageToken;
				bool done = sharedData->pageToken.empty();
				return YieldResult{
					.value = loadBatch,
					.done = done
				};
			});
		});
	}


	Playlist::MutatorDelegate* GoogleDriveStorageProvider::createPlaylistMutatorDelegate($<Playlist> playlist) {
		return new GoogleDrivePlaylistMutatorDelegate(playlist, this);
	}



	#pragma mark Playlist Items

	Promise<GoogleDrivePlaylistItemsPage> GoogleDriveStorageProvider::getPlaylistItems(String uri, size_t offset, size_t limit) {
		return performAsyncJSAPIFunc<GoogleDrivePlaylistItemsPage>("getPlaylistItems", [=](napi_env env) {
			auto optionsObj = Napi::Object::New(env);
			optionsObj.Set("offset", Napi::Number::New(env, (double)offset));
			optionsObj.Set("limit", Napi::Number::New(env, (double)limit));
			return std::vector<napi_value>{
				Napi::String::New(env, uri),
				optionsObj
			};
		}, [=](napi_env env, Napi::Value value) {
			auto jsExports = scripts::getJSExports(env);
			auto json_encode = jsExports.Get("json_encode").As<Napi::Function>();
			auto jsonString = json_encode.Call({ value }).As<Napi::String>().Utf8Value();
			std::string jsonError;
			auto json = Json::parse(jsonString, jsonError);
			if(!jsonError.empty()) {
				throw std::runtime_error("failed to decode json for getPlaylistItems result");
			}
			return GoogleDrivePlaylistItemsPage::fromJson(json, this->options.mediaProviderStash);
		});
	}

	Promise<GoogleDrivePlaylistItemsPage> GoogleDriveStorageProvider::insertPlaylistItems(String uri, size_t index, ArrayList<$<Track>> tracks) {
		return performAsyncJSAPIFunc<GoogleDrivePlaylistItemsPage>("insertPlaylistItems", [=](napi_env env) {
			auto jsExports = scripts::getJSExports(env);
			auto json_decode = jsExports.Get("json_decode").As<Napi::Function>();
			auto tracksJson = Json(tracks.map<Json>([&](auto& track) {
				return track->toJson();
			})).dump();
			auto tracksArray = json_decode.Call({ Napi::String::New(env, tracksJson) }).As<Napi::Object>();
			return std::vector<napi_value>{
				Napi::String::New(env, uri),
				Napi::Number::New(env, (double)index),
				tracksArray
			};
		}, [=](napi_env env, Napi::Value value) {
			auto jsExports = scripts::getJSExports(env);
			auto json_encode = jsExports.Get("json_encode").As<Napi::Function>();
			auto jsonString = json_encode.Call({ value }).As<Napi::String>().Utf8Value();
			std::string jsonError;
			auto json = Json::parse(jsonString, jsonError);
			if(!jsonError.empty()) {
				throw std::runtime_error("failed to decode json for getPlaylistItems result");
			}
			return GoogleDrivePlaylistItemsPage::fromJson(json, this->options.mediaProviderStash);
		});
	}

	Promise<GoogleDrivePlaylistItemsPage> GoogleDriveStorageProvider::appendPlaylistItems(String uri, ArrayList<$<Track>> tracks) {
		return performAsyncJSAPIFunc<GoogleDrivePlaylistItemsPage>("appendPlaylistItems", [=](napi_env env) {
			auto jsExports = scripts::getJSExports(env);
			auto json_decode = jsExports.Get("json_decode").As<Napi::Function>();
			auto tracksJson = Json(tracks.map<Json>([&](auto& track) {
				return track->toJson();
			})).dump();
			auto tracksArray = json_decode.Call({ Napi::String::New(env, tracksJson) }).As<Napi::Object>();
			return std::vector<napi_value>{
				Napi::String::New(env, uri),
				tracksArray
			};
		}, [=](napi_env env, Napi::Value value) {
			auto jsExports = scripts::getJSExports(env);
			auto json_encode = jsExports.Get("json_encode").As<Napi::Function>();
			auto jsonString = json_encode.Call({ value }).As<Napi::String>().Utf8Value();
			std::string jsonError;
			auto json = Json::parse(jsonString, jsonError);
			if(!jsonError.empty()) {
				throw std::runtime_error("failed to decode json for getPlaylistItems result");
			}
			return GoogleDrivePlaylistItemsPage::fromJson(json, this->options.mediaProviderStash);
		});
	}

	Promise<ArrayList<size_t>> GoogleDriveStorageProvider::deletePlaylistItems(String uri, ArrayList<String> itemIds) {
		return performAsyncJSAPIFunc<ArrayList<size_t>>("deletePlaylistItems", [=](napi_env env) {
			auto itemIdsList = Napi::Array::New(env, itemIds.size());
			uint32_t i = 0;
			for(auto& itemId : itemIds) {
				itemIdsList.Set(i, Napi::String::New(env, itemId));
				i++;
			}
			return std::vector<napi_value>{
				Napi::String::New(env, uri),
				itemIdsList
			};
		}, [=](napi_env env, Napi::Value value) {
			ArrayList<size_t> indexes;
			auto indexesArray = value.As<Napi::Object>().Get("indexes").As<Napi::Array>();
			indexes.reserve(indexesArray.Length());
			for(uint32_t i=0; i<indexesArray.Length(); i++) {
				auto index = indexesArray.Get(i).As<Napi::Number>();
				indexes.pushBack((size_t)index.DoubleValue());
			}
			return indexes;
		});
	}

	Promise<void> GoogleDriveStorageProvider::reorderPlaylistItems(String uri, size_t index, size_t count, size_t insertBefore) {
		return performAsyncJSAPIFunc<void>("reorderPlaylistItems", [=](napi_env env) {
			return std::vector<napi_value>{
				Napi::String::New(env, uri),
				Napi::Number::New(env, (double)index),
				Napi::Number::New(env, (double)count),
				Napi::Number::New(env, (double)insertBefore)
			};
		}, nullptr);
	}
}

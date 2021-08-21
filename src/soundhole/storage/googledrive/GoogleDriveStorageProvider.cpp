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
#include <cmath>

namespace sh {
	GoogleDriveStorageProvider::GoogleDriveStorageProvider(MediaItemBuilder* mediaItemBuilder, MediaProviderStash* mediaProviderStash, Options options)
	: jsRef(nullptr), mediaItemBuilder(mediaItemBuilder), mediaProviderStash(mediaProviderStash), options(options) {
		loadSession();
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
			if(!options.redirectURL.empty()) {
				gdOptions.Set("redirectURL", Napi::String::New(env, options.redirectURL));
			}
			gdOptions.Set("appKey", Napi::String::New(env, APP_KEY));
			gdOptions.Set("baseFolderName", Napi::String::New(env, BASE_FOLDER_NAME));
			if(!session.is_null()) {
				gdOptions.Set("session", scripts::parseJsonToNapi(env, session.dump()));
			}
			
			// create media item builder
			if(this->mediaItemBuilder != nullptr) {
				auto mediaItemBuilder = Napi::Object::New(env);
				// name
				mediaItemBuilder.DefineProperty(Napi::PropertyDescriptor::Accessor("name",
					[=](const Napi::CallbackInfo& callbackInfo) -> Napi::Value {
						return Napi::String::New(callbackInfo.Env(),
							this->mediaItemBuilder->name().c_str());
					}));
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
					auto uri = this->mediaItemBuilder->createStorageProviderURI(storageProvider.Utf8Value(), type.Utf8Value(), id.ToString().Utf8Value());
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
					auto uriParts = this->mediaItemBuilder->parseStorageProviderURI(uri.Utf8Value());
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
		return performAsyncFunc<Result>([=](napi_env env) {
			return jsutils::jsValue<Napi::Object>(env, this->jsRef);
		}, funcName, createArgs, mapper, {
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



	#pragma mark Parsing

	GoogleDriveStorageProvider::PlaylistVersionID GoogleDriveStorageProvider::parsePlaylistVersionID(String versionIdString) {
		if(auto date = Date::maybeFromISOString(versionIdString)) {
			return PlaylistVersionID{
				.modifiedAt = date.value()
			};
		}
		throw std::invalid_argument("Invalid playlist version ID format");
	}



	#pragma mark Authentication

	String GoogleDriveStorageProvider::getWebAuthenticationURL(String codeChallenge) const {
		auto query = std::map<String,String>{
			{ "client_id", options.clientId },
			{ "redirect_uri", options.redirectURL },
			{ "response_type", "code" },
			{ "scope", "https://www.googleapis.com/auth/drive" },
			{ "access_type", "offline" }
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
		this->saveSession();
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
		return performAsyncFunc<void>([=](napi_env env) {
			return jsutils::jsValue<Napi::Object>(env, this->jsRef);
		}, "loginWithRedirectParams", [=](napi_env env) {
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
			auto session = sessionFromJS(env);
			if(session.is_null() || !session.is_object()) {
				throw std::runtime_error("Failed to login with authorization code");
			}
			this->session = session;
			this->saveSession();
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
		this->session = Json();
		this->saveSession();
		this->storeIdentity(std::nullopt);
	}

	bool GoogleDriveStorageProvider::isLoggedIn() const {
		return !session.is_null();
	}



	#pragma mark Current User

	Promise<ArrayList<String>> GoogleDriveStorageProvider::getCurrentUserURIs() {
		return getIdentity().map([=](Optional<GoogleDriveStorageProviderUser> user) -> ArrayList<String> {
			if(!user) {
				return ArrayList<String>{};
			}
			return ArrayList<String>{ user->uri };
		});
	}

	String GoogleDriveStorageProvider::getIdentityFilePath() const {
		if(options.sessionPersistKey.empty()) {
			return String();
		}
		return utils::getCacheDirectoryPath()+"/"+name()+"_identity_"+options.sessionPersistKey+".json";
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
			return this->mediaItemBuilder->playlist(
				Playlist::Data::fromJson(json, this->mediaProviderStash));
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
			return Playlist::Data::fromJson(json, this->mediaProviderStash);
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
		return getCurrentUserURIs().then([=](LinkedList<String> userURIs) -> Promise<bool> {
			if(playlist->owner() && userURIs.contains(playlist->owner()->uri())) {
				return Promise<bool>::resolve(true);
			}
			return performAsyncJSAPIFunc<bool>("isPlaylistEditable", [=](napi_env env) {
				return std::vector<napi_value>{
					Napi::String::New(env, uri)
				};
			}, [=](napi_env env, Napi::Value value) {
				return value.As<Napi::Boolean>().Value();
			});
		});
	}



	#pragma mark Playlist Library

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
					throw std::runtime_error("failed to decode json for getUserPlaylists result");
				}
				auto page = GoogleDriveFilesPage<Playlist::Data>::fromJson(json, this->mediaProviderStash);
				auto loadBatch = MediaProvider::LoadBatch<$<Playlist>>{
					.items = page.items.map([=](auto& playlistData) -> $<Playlist> {
						return this->mediaItemBuilder->playlist(playlistData);
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



	Promise<GoogleDriveFilesPage<StorageProvider::LibraryItem>> GoogleDriveStorageProvider::getMyPlaylists(GetMyPlaylistsOptions options) {
		return performAsyncJSAPIFunc<GoogleDriveFilesPage<LibraryItem>>("getMyPlaylists", [=](napi_env env) {
			auto optionsObj = Napi::Object::New(env);
			if(!options.orderBy.empty()) {
				optionsObj.Set("orderBy", Napi::String::New(env, options.orderBy));
			}
			if(!options.pageToken.empty()) {
				optionsObj.Set("pageToken", Napi::String::New(env, options.pageToken));
			}
			if(options.pageSize.hasValue()) {
				optionsObj.Set("pageSize", Napi::Number::New(env, options.pageSize.value()));
			}
			return std::vector<napi_value>{
				optionsObj
			};
		}, [=](napi_env env, Napi::Value value) {
			auto jsExports = scripts::getJSExports(env);
			auto json_encode = jsExports.Get("json_encode").As<Napi::Function>();
			auto jsonString = json_encode.Call({ value }).As<Napi::String>().Utf8Value();
			std::string jsonError;
			auto json = Json::parse(jsonString, jsonError);
			if(!jsonError.empty()) {
				throw std::runtime_error("failed to decode json for getMyPlaylists result");
			}
			return GoogleDriveFilesPage<LibraryItem>{
				.items = jsutils::arrayListFromJson(json["items"], [&](auto& item) -> LibraryItem {
					return LibraryItem{
						.mediaItem = this->mediaItemBuilder->playlist(Playlist::Data::fromJson(item, this->mediaProviderStash)),
						.addedAt = Date::maybeFromISOString(item["createdAt"].string_value())
							.valueOrThrow(std::invalid_argument("Missing field \"createdAt\" for google drive playlist"))
					};
				}),
				.nextPageToken = json["nextPageToken"].string_value()
			};
		});
	}

	Promise<GoogleSheetDBPage<StorageProvider::FollowedItem>> GoogleDriveStorageProvider::getFollowedPlaylists(size_t offset, size_t limit) {
		return performAsyncJSAPIFunc<GoogleSheetDBPage<FollowedItem>>("getFollowedPlaylists", [=](napi_env env) {
			auto optionsObj = Napi::Object::New(env);
			optionsObj.Set("offset", Napi::Number::New(env, offset));
			optionsObj.Set("limit", Napi::Number::New(env, offset));
			return std::vector<napi_value>{
				optionsObj
			};
		}, [=](napi_env env, Napi::Value value) {
			return GoogleSheetDBPage<FollowedItem>::fromNapiObject(value.As<Napi::Object>());
		});
	}

	Promise<GoogleSheetDBPage<StorageProvider::FollowedItem>> GoogleDriveStorageProvider::getFollowedUsers(size_t offset, size_t limit) {
		return performAsyncJSAPIFunc<GoogleSheetDBPage<FollowedItem>>("getFollowedUsers", [=](napi_env env) {
			auto optionsObj = Napi::Object::New(env);
			optionsObj.Set("offset", Napi::Number::New(env, offset));
			optionsObj.Set("limit", Napi::Number::New(env, offset));
			return std::vector<napi_value>{
				optionsObj
			};
		}, [=](napi_env env, Napi::Value value) {
			return GoogleSheetDBPage<FollowedItem>::fromNapiObject(value.As<Napi::Object>());
		});
	}

	Promise<GoogleSheetDBPage<StorageProvider::FollowedItem>> GoogleDriveStorageProvider::getFollowedArtists(size_t offset, size_t limit) {
		return performAsyncJSAPIFunc<GoogleSheetDBPage<FollowedItem>>("getFollowedArtists", [=](napi_env env) {
			auto optionsObj = Napi::Object::New(env);
			optionsObj.Set("offset", Napi::Number::New(env, offset));
			optionsObj.Set("limit", Napi::Number::New(env, offset));
			return std::vector<napi_value>{
				optionsObj
			};
		}, [=](napi_env env, Napi::Value value) {
			return GoogleSheetDBPage<FollowedItem>::fromNapiObject(value.As<Napi::Object>());
		});
	}



	const auto GoogleDriveStorageProvider_syncLibraryTypes = ArrayList<String>{ "my-playlists", "followed-playlists", "followed-artists", "followed_users" };

	Json GoogleDriveStorageProvider::GenerateLibraryResumeData::toJson() const {
		return Json::object{
			{ "mostRecentPlaylistModification", mostRecentPlaylistModification ? (std::string)mostRecentPlaylistModification->toISOString() : Json() },
			{ "mostRecentPlaylistFollow", mostRecentPlaylistFollow ? (std::string)mostRecentPlaylistFollow->toISOString() : Json() },
			{ "mostRecentArtistFollow", mostRecentArtistFollow ? (std::string)mostRecentArtistFollow->toISOString() : Json() },
			{ "mostRecentUserFollow", mostRecentUserFollow ? (std::string)mostRecentUserFollow->toISOString() : Json() },
			{ "syncCurrentType", (std::string)syncCurrentType },
			{ "syncPageToken", (std::string)syncPageToken },
			{ "syncMostRecentSave", syncMostRecentSave ? (std::string)syncMostRecentSave->toISOString() : Json() },
			{ "syncLastItemOffset", syncLastItemOffset ? Json((double)syncLastItemOffset.value()) : Json() },
			{ "syncLastItem", syncLastItem ? syncLastItem->toJson() : Json() }
		};
	}

	GoogleDriveStorageProvider::GenerateLibraryResumeData GoogleDriveStorageProvider::GenerateLibraryResumeData::fromJson(const Json& json) {
		auto mostRecentPlaylistModification = json["mostRecentPlaylistModification"];
		auto mostRecentPlaylistFollow = json["mostRecentPlaylistFollow"];
		auto mostRecentArtistFollow = json["mostRecentArtistFollow"];
		auto mostRecentUserFollow = json["mostRecentUserFollow"];
		auto syncMostRecentSave = json["syncMostRecentSave"];
		auto syncLastItemOffset = json["syncLastItemOffset"];
		auto resumeData = GenerateLibraryResumeData{
			.mostRecentPlaylistModification = mostRecentPlaylistModification.is_string() ? Date::maybeFromISOString(mostRecentPlaylistModification.string_value()) : std::nullopt,
			.mostRecentPlaylistFollow = mostRecentPlaylistFollow.is_string() ? Date::maybeFromISOString(mostRecentPlaylistFollow.string_value()) : std::nullopt,
			.mostRecentArtistFollow = mostRecentArtistFollow.is_string() ? Date::maybeFromISOString(mostRecentArtistFollow.string_value()) : std::nullopt,
			.mostRecentUserFollow = mostRecentUserFollow.is_string() ? Date::maybeFromISOString(mostRecentUserFollow.string_value()) : std::nullopt,
			.syncCurrentType = json["syncCurrentType"].string_value(),
			.syncPageToken = json["syncPageToken"].string_value(),
			.syncMostRecentSave = syncMostRecentSave.is_string() ? Date::maybeFromISOString(syncMostRecentSave.string_value()) : std::nullopt,
			.syncLastItemOffset = syncLastItemOffset.is_number() ? maybe((size_t)syncLastItemOffset.number_value()) : std::nullopt,
			.syncLastItem = Item::maybeFromJson(json["syncLastItem"])
		};
		bool syncTypeValid = GoogleDriveStorageProvider_syncLibraryTypes.contains(resumeData.syncCurrentType);
		if(!(resumeData.syncMostRecentSave.has_value() && resumeData.syncLastItemOffset.has_value()
			 && resumeData.syncLastItem.has_value() && syncTypeValid)) {
			resumeData.syncCurrentType = GoogleDriveStorageProvider_syncLibraryTypes[0];
			resumeData.syncPageToken = String();
			resumeData.syncMostRecentSave = std::nullopt;
			resumeData.syncLastItemOffset = std::nullopt;
			resumeData.syncLastItem = std::nullopt;
		}
		return resumeData;
	}

	Optional<GoogleDriveStorageProvider::GenerateLibraryResumeData::Item> GoogleDriveStorageProvider::GenerateLibraryResumeData::Item::maybeFromJson(const Json& json) {
		if(!json.is_object()) {
			return std::nullopt;
		}
		auto uri = json["uri"].string_value();
		if(uri.empty()) {
			return std::nullopt;
		}
		auto addedAt = Date::maybeFromISOString(json["addedAt"].string_value());
		if(!addedAt) {
			return std::nullopt;
		}
		return Item{
			.uri = uri,
			.addedAt = addedAt.value()
		};
	}

	Json GoogleDriveStorageProvider::GenerateLibraryResumeData::Item::toJson() const {
		return Json::object{
			{ "uri", (std::string)uri },
			{ "addedAt", (std::string)addedAt.toISOString() }
		};
	}



	GoogleDriveStorageProvider::LibraryItemGenerator GoogleDriveStorageProvider::generateLibrary(GenerateLibraryOptions options) {
		struct SharedData {
			GenerateLibraryResumeData resumeData;
			Optional<GoogleSheetDBPage<FollowedItem>> pendingPage;
			bool resuming;
			
			bool attemptRestoreSync(Optional<FollowedItem> firstItem) {
				if(firstItem && resumeData.syncLastItem && firstItem->uri == resumeData.syncLastItem->uri && firstItem->addedAt == resumeData.syncLastItem->addedAt) {
					return true;
				}
				resumeData.syncCurrentType = GoogleDriveStorageProvider_syncLibraryTypes[0];
				resumeData.syncLastItem = std::nullopt;
				resumeData.syncLastItemOffset = std::nullopt;
				resumeData.syncMostRecentSave = std::nullopt;
				resumeData.syncPageToken = String();
				pendingPage = std::nullopt;
				return false;
			}
		};
		auto sharedData = fgl::new$<SharedData>(SharedData{
			.resumeData = GenerateLibraryResumeData::fromJson(options.resumeData),
			.resuming = true
		});
		using YieldResult = LibraryItemGenerator::YieldResult;
		return LibraryItemGenerator(coLambda([=]() -> Promise<YieldResult> {
			co_await resumeOnQueue(DispatchQueue::main());
			auto& resumeData = sharedData->resumeData;
			if(resumeData.syncCurrentType == "my-playlists") {
				// sync playlists created by the current user
				GoogleDriveFilesPage<LibraryItem> page;
				auto pageToken = resumeData.syncPageToken;
				if(sharedData->resuming && !pageToken.empty()) {
					sharedData->resuming = false;
					// ensure page token didn't get invalidated
					try {
						page = co_await this->getMyPlaylists({
							.pageToken = pageToken,
							.pageSize = 1000,
							.orderBy = "modifiedTime desc"
						});
					} catch(...) {
						// page token was invalid, so start over
						resumeData.syncPageToken = String();
						resumeData.syncLastItem = std::nullopt;
						resumeData.syncLastItemOffset = std::nullopt;
						resumeData.syncMostRecentSave = std::nullopt;
						co_return YieldResult{
							.value = GenerateLibraryResults{
								.resumeData = resumeData.toJson(),
								.progress = 0,
								.items = {}
							},
							.done = false
						};
					}
				} else {
					sharedData->resuming = false;
					page = co_await this->getMyPlaylists({
						.pageToken = pageToken,
						.pageSize = 1000,
						.orderBy = "modifiedTime desc"
					});
				}
				size_t offset = resumeData.syncLastItemOffset.valueOr(0) + page.items.size();
				bool sectionDone = page.nextPageToken.empty();
				// check if sync is caught up with the previous sync's most recent modification date
				if(!sectionDone && resumeData.mostRecentPlaylistModification && !page.items.empty()) {
					auto lastItem = std::static_pointer_cast<Playlist>(page.items.back().mediaItem);
					auto versionId = this->parsePlaylistVersionID(lastItem->versionId());
					// check if last item was modified earlier than the most recent modification date
					if(versionId.modifiedAt <= resumeData.mostRecentPlaylistModification.value()) {
						sectionDone = true;
					}
				}
				// apply syncMostRecentSave if we're at the first chunk
				if(pageToken.empty() && !page.items.empty()) {
					auto firstItem = std::static_pointer_cast<Playlist>(page.items.front().mediaItem);
					auto versionId = this->parsePlaylistVersionID(firstItem->versionId());
					resumeData.syncMostRecentSave = versionId.modifiedAt;
				}
				// update resume data based on state
				if(sectionDone) {
					resumeData.syncCurrentType = GoogleDriveStorageProvider_syncLibraryTypes[1];
					if(resumeData.syncMostRecentSave) {
						resumeData.mostRecentPlaylistModification = resumeData.syncMostRecentSave;
					}
					resumeData.syncMostRecentSave = std::nullopt;
					resumeData.syncLastItem = std::nullopt;
					resumeData.syncLastItemOffset = std::nullopt;
					resumeData.syncPageToken = String();
				} else {
					resumeData.syncLastItemOffset = offset;
					resumeData.syncLastItem = std::nullopt;
					resumeData.syncPageToken = page.nextPageToken;
				}
				co_return YieldResult{
					.value = GenerateLibraryResults{
						.resumeData = resumeData.toJson(),
						.progress = (sectionDone ? 1.0 : std::min(1.0, (double)offset / 10000.0)) / (double)GoogleDriveStorageProvider_syncLibraryTypes.size(),
						.items = page.items
					},
					.done = sectionDone
				};
			}
			else {
				// sync items followed by the current user
				auto generateFollowedItems = [=](
						Optional<Date>* mostRecentSave,
						auto itemsGetter,
						auto mediaItemsGetter,
						auto mediaItemGetter) -> Promise<YieldResult> {
					bool resuming = sharedData->resuming;
					auto& resumeData = sharedData->resumeData;
					size_t sectionIndex = GoogleDriveStorageProvider_syncLibraryTypes.indexOf(resumeData.syncCurrentType);
					// get page of followed items
					size_t offset;
					if(!sharedData->pendingPage || sharedData->pendingPage->items.size() == 0) {
						// load items from API and set pending page
						offset = resumeData.syncLastItemOffset.valueOr(0);
						size_t limit = 10000;
						sharedData->pendingPage = co_await itemsGetter(offset, limit);
						if(resuming && offset > 0) {
							auto lastItem = (sharedData->pendingPage->items.size() > 0) ? maybe(sharedData->pendingPage->items.at(0)) : std::nullopt;
							if(!sharedData->attemptRestoreSync(lastItem)) {
								sharedData->resuming = false;
								co_return YieldResult{
									.value = GenerateLibraryResults{
										.resumeData = resumeData.toJson(),
										.items = {},
										.progress = (double)sectionIndex / (double)GoogleDriveStorageProvider_syncLibraryTypes.size()
									},
									.done = false
								};
							}
						}
					} else {
						// just dequeue items from pending page
						offset = sharedData->pendingPage->offset;
					}
					sharedData->resuming = false;
					// construct smaller page and dequeue items
					GoogleSheetDBPage<FollowedItem> page;
					size_t chunkSize = 50;
					page.offset = sharedData->pendingPage->offset;
					page.total = sharedData->pendingPage->total;
					page.items = sharedData->pendingPage->items.slice(0, chunkSize);
					// organize lists of items for each provider in background thread
					co_await resumeOnNewThread();
					struct ItemList {
						MediaProvider* provider;
						LinkedList<std::pair<size_t,FollowedItem>> items;
					};
					auto listsToFetch = LinkedList<ItemList>();
					size_t i=0;
					for(auto& item : page.items) {
						auto provider = mediaProviderStash->getMediaProvider(item.provider);
						auto listIt = listsToFetch.findWhere([&](auto& list) {
							return list.provider == provider;
						});
						if(listIt == listsToFetch.end()) {
							// list doesn't exist, so it needs to be added
							listsToFetch.pushBack(ItemList{
								.provider = provider,
								.items = { std::make_pair(i, item) }
							});
						} else {
							// list exists, so add to it
							listIt->items.pushBack(std::make_pair(i, item));
						}
						i++;
					}
					co_await resumeOnQueue(DispatchQueue::main());
					// fetch items for each provider
					using FetchedItemList = ArrayList<std::pair<size_t,LibraryItem>>;
					auto fetchedItemLists = co_await Promise<FetchedItemList>::all(ArrayList<Promise<FetchedItemList>>(listsToFetch.map([=](ItemList list) -> Promise<FetchedItemList> {
						if(list.items.size() == 1) {
							auto& pair = list.items.front();
							return mediaItemGetter(list.provider, pair.second.uri).map([=](auto mediaItem) -> FetchedItemList {
								return FetchedItemList{ std::make_pair(pair.first, LibraryItem{
									.mediaItem = mediaItem,
									.addedAt = pair.second.addedAt
								}) };
							});
						} else {
							auto uris = list.items.map([](auto& pair) {
								return pair.second.uri;
							});
							return mediaItemsGetter(list.provider, uris).map([=](auto mediaItems) -> FetchedItemList {
								FetchedItemList mappedLibraryItems;
								mappedLibraryItems.reserve(mediaItems.size());
								auto itemIt = list.items.begin();
								for(auto& mediaItem : mediaItems) {
									mappedLibraryItems.pushBack(std::make_pair(itemIt->first, LibraryItem{
										.mediaItem = mediaItem,
										.addedAt = itemIt->second.addedAt
									}));
									itemIt++;
								}
								return mappedLibraryItems;
							});
						}
					})));
					// map items back into single list
					ArrayList<LibraryItem> items;
					{
						ArrayList<Optional<LibraryItem>> optItems;
						optItems.resize(page.items.size());
						for(auto& itemList : fetchedItemLists) {
							for(auto& pair : itemList) {
								optItems[pair.first] = pair.second;
							}
						}
						items = optItems.map([](auto& val) { return val.value(); });
					}
					// apply syncMostRecentSave if we're at 0
					if(offset == 0 && page.items.size() > 0) {
						resumeData.syncMostRecentSave = page.items.front().addedAt;
					}
					// check if we're caught up
					bool caughtUp = false;
					if(*mostRecentSave) {
						for(auto& item : page.items) {
							if(item.addedAt <= *mostRecentSave) {
								caughtUp = true;
								break;
							}
						}
					}
					// determine progress
					bool lastSection = (sectionIndex == (GoogleDriveStorageProvider_syncLibraryTypes.size() - 1));
					bool sectionDone = (page.offset + page.items.size()) >= page.total;
					bool done = false;
					if(sectionDone || caughtUp) {
						if(lastSection) {
							done = true;
							resumeData.syncCurrentType = String();
						} else {
							resumeData.syncCurrentType = GoogleDriveStorageProvider_syncLibraryTypes[sectionIndex + 1];
						}
						if(resumeData.syncMostRecentSave) {
							*mostRecentSave = resumeData.syncMostRecentSave;
						}
						resumeData.syncLastItem = std::nullopt;
						resumeData.syncLastItemOffset = std::nullopt;
						resumeData.syncPageToken = String();
						resumeData.syncMostRecentSave = std::nullopt;
						sharedData->pendingPage = std::nullopt;
					} else {
						sharedData->pendingPage->items = sharedData->pendingPage->items.slice(items.size());
						sharedData->pendingPage->offset += items.size();
						if(sharedData->pendingPage->items.empty()) {
							sharedData->pendingPage = std::nullopt;
						}
						resumeData.syncPageToken = String();
						if(items.size() > 0) {
							auto& lastItem = items.back();
							resumeData.syncLastItem = GenerateLibraryResumeData::Item{
								.uri = lastItem.mediaItem->uri(),
								.addedAt = lastItem.addedAt
							};
							resumeData.syncLastItemOffset = offset + (items.size() - 1);
						}
					}
					co_return YieldResult{
						.value = GenerateLibraryResults{
							.resumeData = resumeData.toJson(),
							.items = items,
							.progress = ((double)sectionIndex + ((double)(offset + items.size()) / (double)page.total)) / (double)GoogleDriveStorageProvider_syncLibraryTypes.size()
						},
						.done = done
					};
				};
				
				if(resumeData.syncCurrentType == "followed-playlists") {
					co_return co_await generateFollowedItems(&resumeData.mostRecentPlaylistFollow, [=](size_t offset, size_t limit) {
						return getFollowedPlaylists(offset, limit);
					}, [](MediaProvider* provider, ArrayList<String> uris) {
						return provider->getPlaylists(uris);
					}, [](MediaProvider* provider, String uri) {
						return provider->getPlaylist(uri);
					});
				}
				else if(resumeData.syncCurrentType == "followed-users") {
					co_return co_await generateFollowedItems(&resumeData.mostRecentUserFollow, [=](size_t offset, size_t limit) {
						return getFollowedUsers(offset, limit);
					}, [](MediaProvider* provider, ArrayList<String> uris) {
						return provider->getUsers(uris);
					}, [](MediaProvider* provider, String uri) {
						return provider->getUser(uri);
					});
				}
				else if(resumeData.syncCurrentType == "followed-artists") {
					co_return co_await generateFollowedItems(&resumeData.mostRecentArtistFollow, [=](size_t offset, size_t limit) {
						return getFollowedArtists(offset, limit);
					}, [](MediaProvider* provider, ArrayList<String> uris) {
						return provider->getArtists(uris);
					}, [](MediaProvider* provider, String uri) {
						return provider->getArtist(uri);
					});
				}
				else {
					throw std::runtime_error("invalid sync type "+resumeData.syncCurrentType);
				}
			}
		}));
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
			return GoogleDrivePlaylistItemsPage::fromJson(json, this->mediaProviderStash);
		});
	}

	Promise<GoogleDrivePlaylistItemsPage> GoogleDriveStorageProvider::insertPlaylistItems(String uri, size_t index, ArrayList<$<Track>> tracks) {
		return performAsyncJSAPIFunc<GoogleDrivePlaylistItemsPage>("insertPlaylistItems", [=](napi_env env) {
			auto jsExports = scripts::getJSExports(env);
			auto json_decode = jsExports.Get("json_decode").As<Napi::Function>();
			auto tracksJson = Json(tracks.map([&](auto& track) -> Json {
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
			return GoogleDrivePlaylistItemsPage::fromJson(json, this->mediaProviderStash);
		});
	}

	Promise<GoogleDrivePlaylistItemsPage> GoogleDriveStorageProvider::appendPlaylistItems(String uri, ArrayList<$<Track>> tracks) {
		return performAsyncJSAPIFunc<GoogleDrivePlaylistItemsPage>("appendPlaylistItems", [=](napi_env env) {
			auto jsExports = scripts::getJSExports(env);
			auto json_decode = jsExports.Get("json_decode").As<Napi::Function>();
			auto tracksJson = Json(tracks.map([&](auto& track) -> Json {
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
			return GoogleDrivePlaylistItemsPage::fromJson(json, this->mediaProviderStash);
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

	Promise<void> GoogleDriveStorageProvider::movePlaylistItems(String uri, size_t index, size_t count, size_t insertBefore) {
		return performAsyncJSAPIFunc<void>("movePlaylistItems", [=](napi_env env) {
			return std::vector<napi_value>{
				Napi::String::New(env, uri),
				Napi::Number::New(env, (double)index),
				Napi::Number::New(env, (double)count),
				Napi::Number::New(env, (double)insertBefore)
			};
		}, nullptr);
	}



	#pragma mark Playlist Following / Unfollowing

	Promise<void> GoogleDriveStorageProvider::followPlaylists(ArrayList<NewFollowedItem> playlists) {
		return performAsyncJSAPIFunc<void>("followPlaylists", [=](napi_env env) {
			auto objs = Napi::Array::New(env);
			for(auto& playlist : playlists) {
				objs.Get("push").As<Napi::Function>().Call(objs, { playlist.toNapiObject(env) });
			}
			return std::vector<napi_value>{ objs };
		}, nullptr);
	}

	Promise<void> GoogleDriveStorageProvider::unfollowPlaylists(ArrayList<String> playlistURIs) {
		return performAsyncJSAPIFunc<void>("unfollowPlaylists", [=](napi_env env) {
			auto objs = Napi::Array::New(env);
			for(auto uri : playlistURIs) {
				objs.Get("push").As<Napi::Function>().Call(objs, { uri.toNodeJSValue(env) });
			}
			return std::vector<napi_value>{ objs };
		}, nullptr);
	}



	#pragma mark Artist Following / Unfollowing

	Promise<void> GoogleDriveStorageProvider::followArtists(ArrayList<NewFollowedItem> artists) {
		return performAsyncJSAPIFunc<void>("followArtists", [=](napi_env env) {
			auto objs = Napi::Array::New(env);
			for(auto& artist : artists) {
				objs.Get("push").As<Napi::Function>().Call(objs, { artist.toNapiObject(env) });
			}
			return std::vector<napi_value>{ objs };
		}, nullptr);
	}

	Promise<void> GoogleDriveStorageProvider::unfollowArtists(ArrayList<String> artistURIs) {
		return performAsyncJSAPIFunc<void>("unfollowArtists", [=](napi_env env) {
			auto objs = Napi::Array::New(env);
			for(auto uri : artistURIs) {
				objs.Get("push").As<Napi::Function>().Call(objs, { uri.toNodeJSValue(env) });
			}
			return std::vector<napi_value>{ objs };
		}, nullptr);
	}



	#pragma mark User Following / Unfollowing

	Promise<void> GoogleDriveStorageProvider::followUsers(ArrayList<NewFollowedItem> users) {
		return performAsyncJSAPIFunc<void>("followUsers", [=](napi_env env) {
			auto objs = Napi::Array::New(env);
			for(auto& user : users) {
				objs.Get("push").As<Napi::Function>().Call(objs, { user.toNapiObject(env) });
			}
			return std::vector<napi_value>{ objs };
		}, nullptr);
	}

	Promise<void> GoogleDriveStorageProvider::unfollowUsers(ArrayList<String> userURIs) {
		return performAsyncJSAPIFunc<void>("unfollowUsers", [=](napi_env env) {
			auto objs = Napi::Array::New(env);
			for(auto uri : userURIs) {
				objs.Get("push").As<Napi::Function>().Call(objs, { uri.toNodeJSValue(env) });
			}
			return std::vector<napi_value>{ objs };
		}, nullptr);
	}
}

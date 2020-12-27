//
//  Bandcamp.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/16/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include <napi.h>
#include "Bandcamp.hpp"
#include "BandcampError.hpp"
#include <soundhole/scripts/Scripts.hpp>
#include <soundhole/utils/js/JSUtils.hpp>
#include <soundhole/utils/js/JSWrapClass.impl.hpp>

namespace sh {
	Bandcamp::Bandcamp(Options options)
	: jsRef(nullptr), auth(new BandcampAuth(options.auth)) {
		auth->load();
	}

	Bandcamp::~Bandcamp() {
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
		delete auth;
	}

	void Bandcamp::initializeJS(napi_env env) {
		if(this->jsRef == nullptr) {
			// create auth options
			auto authOptions = Napi::Object::New(env);
			
			// create sessionCookies
			auto session = auth->getSession();
			if(session) {
				auto sessionCookies = Napi::Array::New(env);
				auto sessionCookiesPush = sessionCookies.Get("push").As<Napi::Function>();
				for(auto& cookie : session->getCookies()) {
					sessionCookiesPush.Call(sessionCookies, { Napi::String::New(env, cookie.c_str()) });
				}
				authOptions.Set("sessionCookies", sessionCookies);
			}
			
			// create options
			auto bcOptions = Napi::Object::New(env);
			bcOptions.Set("auth", authOptions);
			
			// instantiate bandcamp
			auto jsExports = scripts::getJSExports(env);
			auto BandcampClass = jsExports.Get("Bandcamp").As<Napi::Function>();
			auto bandcamp = BandcampClass.New({ bcOptions });
			auto bandcampRef = Napi::ObjectReference::New(bandcamp, 1);
			bandcampRef.SuppressDestruct();
			jsRef = bandcampRef;
		}
	}



	BandcampAuth* Bandcamp::getAuth() {
		return auth;
	}

	const BandcampAuth* Bandcamp::getAuth() const {
		return auth;
	}

	Promise<bool> Bandcamp::login() {
		return auth->login().map<bool>([=](bool loggedIn) {
			queueUpdateJSSession(auth->getSession());
			return loggedIn;
		});
	}

	void Bandcamp::logout() {
		auth->logout();
		queueUpdateJSSession(auth->getSession());
	}

	bool Bandcamp::isLoggedIn() const {
		return auth->isLoggedIn();
	}



	void Bandcamp::updateJSSession(napi_env env, Optional<BandcampSession> session) {
		auto jsApi = jsutils::jsValue<Napi::Object>(env, jsRef);
		if(jsApi.IsEmpty() || jsApi.IsNull() || jsApi.IsEmpty()) {
			printf("jsApi is null in Bandcamp::updateJSSession");
			return;
		}
		if(!session) {
			// logout if null session
			jsApi.Get("logout").As<Napi::Function>().Call(jsApi, {});
			return;
		}
		
		// create session cookies
		auto sessionCookies = Napi::Array::New(env);
		auto sessionCookiesPush = sessionCookies.Get("push").As<Napi::Function>();
		for(auto& cookie : session->getCookies()) {
			sessionCookiesPush.Call(sessionCookies, { Napi::String::New(env, cookie.c_str()) });
		}
		
		// login with cookies
		auto existingSession = jsApi.Get("session").As<Napi::Object>();
		if(existingSession.IsEmpty() || existingSession.IsNull() || existingSession.IsUndefined()) {
			jsApi.Get("loginWithCookies").As<Napi::Function>().Call(jsApi, { sessionCookies });
		} else {
			jsApi.Get("updateSessionCookies").As<Napi::Function>().Call(jsApi, { sessionCookies });
		}
		
		// TODO if session isn't logged in, log out this session
	}

	void Bandcamp::queueUpdateJSSession(Optional<BandcampSession> session) {
		queueJS([=](napi_env env) {
			updateJSSession(env, session);
		});
	}

	void Bandcamp::updateSessionFromJS(napi_env env) {
		auto jsApi = jsutils::jsValue<Napi::Object>(env, jsRef);
		if(jsApi.IsEmpty() || jsApi.IsNull() || jsApi.IsEmpty()) {
			printf("jsApi is null in Bandcamp::updateSessionFromJS");
			return;
		}
		auto sessionJs = jsApi.Get("session").As<Napi::Object>();
		if(sessionJs.IsEmpty() || sessionJs.IsNull() || sessionJs.IsUndefined()) {
			auto existingSession = auth->getSession();
			if(existingSession) {
				updateJSSession(env, existingSession);
			}
			return;
		}
		auto sessionJsonStr = sessionJs.Get("serialize").As<Napi::Function>().Call(sessionJs, {}).As<Napi::String>().Utf8Value();
		std::string jsonError;
		auto sessionJson = Json::parse(sessionJsonStr, jsonError);
		auto session = BandcampSession::fromJson(sessionJson);
		if(!session) {
			if(auth->isLoggedIn()) {
				auth->logout();
			}
			return;
		}
		if(session != auth->getSession()) {
			auth->loginWithSession(session.value());
		}
	}



	template<typename Result>
	Promise<Result> Bandcamp::performAsyncBandcampJSFunc(String funcName, Function<std::vector<napi_value>(napi_env)> createArgs, Function<Result(napi_env,Napi::Value)> mapper) {
		return performAsyncFunc<Result>(jsRef, funcName, createArgs, mapper, {
			.beforeFuncCall = [=](napi_env env) {
				// update session before function call
				if(auto session = this->auth->getSession()) {
					this->updateJSSession(env, session);
				}
			},
			.afterFuncFinish = [=](napi_env env) {
				// update session after function finishes
				this->updateSessionFromJS(env);
			}
		});
	}



	Promise<BandcampIdentities> Bandcamp::getMyIdentities() {
		return performAsyncBandcampJSFunc<BandcampIdentities>("getMyIdentities", [](napi_env) {
			return std::vector<napi_value>{};
		}, [](napi_env env, Napi::Value value) {
			return BandcampIdentities::fromNapiObject(value.As<Napi::Object>());
		});
	}



	Promise<BandcampSearchResults> Bandcamp::search(String query, SearchOptions options) {
		return performAsyncBandcampJSFunc<BandcampSearchResults>("search", [=](napi_env env) {
			auto jsOptions = Napi::Object::New(env);
			jsOptions.Set("page", Napi::Number::New(env, (double)options.page));
			return std::vector<napi_value>{
				query.toNodeJSValue(env),
				jsOptions
			};
		}, [](napi_env env, Napi::Value value) {
			return BandcampSearchResults::fromNapiObject(value.As<Napi::Object>());
		});
	}

	Promise<BandcampTrack> Bandcamp::getTrack(String url) {
		return performAsyncBandcampJSFunc<BandcampTrack>("getTrack", [=](napi_env env) {
			return std::vector<napi_value>{
				url.toNodeJSValue(env)
			};
		}, [](napi_env env, Napi::Value value) {
			Napi::Object obj = value.As<Napi::Object>();
			auto mediaType = obj.Get("type").ToString().Utf8Value();
			if(mediaType != "track") {
				throw BandcampError(BandcampError::Code::MEDIATYPE_MISMATCH, "Bandcamp item is " + mediaType + ", not track");
			}
			return BandcampTrack::fromNapiObject(value.As<Napi::Object>());
		});
	}

	Promise<BandcampAlbum> Bandcamp::getAlbum(String url) {
		return performAsyncBandcampJSFunc<BandcampAlbum>("getAlbum", [=](napi_env env) {
			return std::vector<napi_value>{
				url.toNodeJSValue(env)
			};
		}, [](napi_env env, Napi::Value value) {
			Napi::Object obj = value.As<Napi::Object>();
			auto mediaType = obj.Get("type").ToString().Utf8Value();
			if(mediaType == "track") {
				// parse bandcamp single
				BandcampTrack track = BandcampTrack::fromNapiObject(obj);
				if(track.url.empty()) {
					throw std::runtime_error("missing URL for bandcamp album");
				} else if(track.url != track.albumURL) {
					throw BandcampError(BandcampError::Code::MEDIATYPE_MISMATCH, "Bandcamp item is " + mediaType + ", not album");
				}
				return BandcampAlbum::fromSingle(track);
			} else if(mediaType == "album") {
				// parse bandcamp album
				return BandcampAlbum::fromNapiObject(obj);
			} else {
				// invalid type
				throw BandcampError(BandcampError::Code::MEDIATYPE_MISMATCH, "Bandcamp item is " + mediaType + ", not album");
			}
		});
	}

	Promise<BandcampArtist> Bandcamp::getArtist(String url) {
		return performAsyncBandcampJSFunc<BandcampArtist>("getArtist", [=](napi_env env) {
			return std::vector<napi_value>{
				url.toNodeJSValue(env)
			};
		}, [](napi_env env, Napi::Value value) {
			Napi::Object obj = value.As<Napi::Object>();
			auto mediaType = obj.Get("type").ToString().Utf8Value();
			if(mediaType != "artist" && mediaType != "label") {
				throw BandcampError(BandcampError::Code::MEDIATYPE_MISMATCH, "Bandcamp item is " + mediaType + ", not artist or label");
			}
			return BandcampArtist::fromNapiObject(obj);
		});
	}

	Promise<BandcampFan> Bandcamp::getFan(String url) {
		return performAsyncBandcampJSFunc<BandcampFan>("getFan", [=](napi_env env) {
			return std::vector<napi_value>{
				url.toNodeJSValue(env)
			};
		}, [](napi_env env, Napi::Value value) {
			Napi::Object obj = value.As<Napi::Object>();
			auto mediaType = obj.Get("type").ToString().Utf8Value();
			if(mediaType != "fan") {
				throw BandcampError(BandcampError::Code::MEDIATYPE_MISMATCH, "Bandcamp item is " + mediaType + ", not fan");
			}
			return BandcampFan::fromNapiObject(obj);
		});
	}



	Promise<BandcampFanSectionPage<BandcampFan::CollectionItemNode>> Bandcamp::getFanCollectionItems(String fanURL, String fanId, GetFanSectionItemsOptions options) {
		return performAsyncBandcampJSFunc<BandcampFanSectionPage<BandcampFan::CollectionItemNode>>("getFanCollectionItems", [=](napi_env env) {
			auto optionsObj = Napi::Object::New(env);
			if(!options.olderThanToken.empty()) {
				optionsObj["olderThanToken"] = options.olderThanToken.toNodeJSValue(env);
			}
			if(!options.count.hasValue()) {
				optionsObj["count"] = Napi::Number::New(env, (double)options.count.value());
			}
			return std::vector<napi_value>{
				fanURL.toNodeJSValue(env),
				fanId.toNodeJSValue(env),
				optionsObj
			};
		}, [](napi_env env, Napi::Value value) {
			Napi::Object obj = value.As<Napi::Object>();
			return BandcampFanSectionPage<BandcampFan::CollectionItemNode>::fromNapiObject(obj);
		});
	}


	Promise<BandcampFanSectionPage<BandcampFan::CollectionItemNode>> Bandcamp::getFanWishlistItems(String fanURL, String fanId, GetFanSectionItemsOptions options) {
		return performAsyncBandcampJSFunc<BandcampFanSectionPage<BandcampFan::CollectionItemNode>>("getFanWishlistItems", [=](napi_env env) {
			auto optionsObj = Napi::Object::New(env);
			if(!options.olderThanToken.empty()) {
				optionsObj["olderThanToken"] = options.olderThanToken.toNodeJSValue(env);
			}
			if(!options.count.hasValue()) {
				optionsObj["count"] = Napi::Number::New(env, (double)options.count.value());
			}
			return std::vector<napi_value>{
				fanURL.toNodeJSValue(env),
				fanId.toNodeJSValue(env),
				optionsObj
			};
		}, [](napi_env env, Napi::Value value) {
			Napi::Object obj = value.As<Napi::Object>();
			return BandcampFanSectionPage<BandcampFan::CollectionItemNode>::fromNapiObject(obj);
		});
	}


	Promise<BandcampFanSectionPage<BandcampFan::CollectionItemNode>> Bandcamp::getFanHiddenItems(String fanURL, String fanId, GetFanSectionItemsOptions options) {
		return performAsyncBandcampJSFunc<BandcampFanSectionPage<BandcampFan::CollectionItemNode>>("getFanHiddenItems", [=](napi_env env) {
			auto optionsObj = Napi::Object::New(env);
			if(!options.olderThanToken.empty()) {
				optionsObj["olderThanToken"] = options.olderThanToken.toNodeJSValue(env);
			}
			if(!options.count.hasValue()) {
				optionsObj["count"] = Napi::Number::New(env, (double)options.count.value());
			}
			return std::vector<napi_value>{
				fanURL.toNodeJSValue(env),
				fanId.toNodeJSValue(env),
				optionsObj
			};
		}, [](napi_env env, Napi::Value value) {
			Napi::Object obj = value.As<Napi::Object>();
			return BandcampFanSectionPage<BandcampFan::CollectionItemNode>::fromNapiObject(obj);
		});
	}
}

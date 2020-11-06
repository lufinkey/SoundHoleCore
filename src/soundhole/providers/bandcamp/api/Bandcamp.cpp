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



	Promise<BandcampSearchResults> Bandcamp::search(String query, SearchOptions options) {
		return Promise<BandcampSearchResults>([=](auto resolve, auto reject) {
			queueJS([=](napi_env env) {
				// update session
				if(auto session = auth->getSession()) {
					updateJSSession(env, session);
				}
				// get api object and make call
				auto jsApi = jsutils::jsValue<Napi::Object>(env, jsRef);
				if(jsApi.IsEmpty()) {
					reject(BandcampError(BandcampError::Code::NOT_INITIALIZED, "Bandcamp not initialized"));
					return;
				}
				auto jsOptions = Napi::Object::New(env);
				jsOptions.Set("page", Napi::Number::New(env, (double)options.page));
				auto promise = jsApi.Get("search").As<Napi::Function>().Call(jsApi, {
					query.toNodeJSValue(env),
					jsOptions
				}).As<Napi::Object>();
				promise.Get("then").As<Napi::Function>().Call(promise, {
					// then
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						updateSessionFromJS(info.Env());
						// decode search results
						resolve(BandcampSearchResults::fromNapiObject(info[0].As<Napi::Object>()));
					}),
					// catch
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						updateSessionFromJS(info.Env());
						// handle error
						auto errorMessage = info[0].As<Napi::Object>().Get("message").ToString();
						reject(BandcampError(BandcampError::Code::REQUEST_FAILED, errorMessage));
					})
				});
			});
		});
	}

	Promise<BandcampTrack> Bandcamp::getTrack(String url) {
		return Promise<BandcampTrack>([=](auto resolve, auto reject) {
			queueJS([=](napi_env env) {
				// update session
				if(auto session = auth->getSession()) {
					updateJSSession(env, session);
				}
				// get api object and make call
				auto jsApi = jsutils::jsValue<Napi::Object>(env, jsRef);
				if(jsApi.IsEmpty()) {
					reject(BandcampError(BandcampError::Code::NOT_INITIALIZED, "Bandcamp not initialized"));
					return;
				}
				auto promise = jsApi.Get("getTrack").As<Napi::Function>().Call(jsApi, {
					url.toNodeJSValue(env),
				}).As<Napi::Object>();
				promise.Get("then").As<Napi::Function>().Call(promise, {
					// then
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						updateSessionFromJS(info.Env());
						// decode track
						auto obj = info[0].As<Napi::Object>();
						auto mediaType = obj.Get("type").ToString().Utf8Value();
						if(mediaType != "track") {
							reject(BandcampError(BandcampError::Code::MEDIATYPE_MISMATCH, "Bandcamp item is " + mediaType + ", not track"));
						} else {
							resolve(BandcampTrack::fromNapiObject(obj));
						}
					}),
					// catch
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						updateSessionFromJS(info.Env());
						// handle error
						auto errorMessage = info[0].As<Napi::Object>().Get("message").ToString();
						reject(BandcampError(BandcampError::Code::REQUEST_FAILED, errorMessage));
					})
				});
			});
		});
	}

	Promise<BandcampAlbum> Bandcamp::getAlbum(String url) {
		return Promise<BandcampAlbum>([=](auto resolve, auto reject) {
			queueJS([=](napi_env env) {
				// update session
				if(auto session = auth->getSession()) {
					updateJSSession(env, session);
				}
				// get api object and make call
				auto jsApi = jsutils::jsValue<Napi::Object>(env, jsRef);
				if(jsApi.IsEmpty()) {
					reject(BandcampError(BandcampError::Code::NOT_INITIALIZED, "Bandcamp not initialized"));
					return;
				}
				auto promise = jsApi.Get("getAlbum").As<Napi::Function>().Call(jsApi, {
					url.toNodeJSValue(env),
				}).As<Napi::Object>();
				promise.Get("then").As<Napi::Function>().Call(promise, {
					// then
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						updateSessionFromJS(info.Env());
						//decode album
						auto obj = info[0].As<Napi::Object>();
						auto mediaType = obj.Get("type").ToString().Utf8Value();
						if(mediaType == "track") {
							auto track = BandcampTrack::fromNapiObject(obj);
							if(track.url.empty() || track.url != track.albumURL) {
								reject(BandcampError(BandcampError::Code::MEDIATYPE_MISMATCH, "Bandcamp item is " + mediaType + ", not album"));
							}
							BandcampAlbum album;
							try {
								album = BandcampAlbum::fromSingle(track);
							} catch(...) {
								reject(std::current_exception());
								return;
							}
							resolve(album);
						} else if(mediaType != "album") {
							reject(BandcampError(BandcampError::Code::MEDIATYPE_MISMATCH, "Bandcamp item is " + mediaType + ", not album"));
						} else {
							resolve(BandcampAlbum::fromNapiObject(obj));
						}
					}),
					// catch
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						updateSessionFromJS(info.Env());
						// handle error
						auto errorMessage = info[0].As<Napi::Object>().Get("message").ToString();
						reject(BandcampError(BandcampError::Code::REQUEST_FAILED, errorMessage));
					})
				});
			});
		});
	}

	Promise<BandcampArtist> Bandcamp::getArtist(String url) {
		return Promise<BandcampArtist>([=](auto resolve, auto reject) {
			queueJS([=](napi_env env) {
				// update session
				if(auto session = auth->getSession()) {
					updateJSSession(env, session);
				}
				// get api object and make call
				auto jsApi = jsutils::jsValue<Napi::Object>(env, jsRef);
				if(jsApi.IsEmpty()) {
					reject(BandcampError(BandcampError::Code::NOT_INITIALIZED, "Bandcamp not initialized"));
					return;
				}
				auto promise = jsApi.Get("getArtist").As<Napi::Function>().Call(jsApi, {
					url.toNodeJSValue(env),
				}).As<Napi::Object>();
				promise.Get("then").As<Napi::Function>().Call(promise, {
					// then
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						updateSessionFromJS(info.Env());
						// decode artist
						auto obj = info[0].As<Napi::Object>();
						auto mediaType = obj.Get("type").ToString().Utf8Value();
						if(mediaType != "artist" && mediaType != "label") {
							reject(BandcampError(BandcampError::Code::MEDIATYPE_MISMATCH, "Bandcamp item is " + mediaType + ", not artist or label"));
						} else {
							resolve(BandcampArtist::fromNapiObject(obj));
						}
					}),
					// catch
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						updateSessionFromJS(info.Env());
						// handle error
						auto errorMessage = info[0].As<Napi::Object>().Get("message").ToString();
						reject(BandcampError(BandcampError::Code::REQUEST_FAILED, errorMessage));
					})
				});
			});
		});
	}
}

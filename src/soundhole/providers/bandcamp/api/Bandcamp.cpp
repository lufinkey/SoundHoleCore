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
	Bandcamp::Bandcamp()
	: jsRef(nullptr) {
		//
	}

	Bandcamp::~Bandcamp() {
		queueJSDestruct([=](napi_env env) {
			auto napiRef = Napi::ObjectReference(env, jsRef);
			if(napiRef.Unref() == 0) {
				napiRef.Reset();
			} else {
				napiRef.SuppressDestruct();
			}
		});
	}

	void Bandcamp::initializeJS(napi_env env) {
		if(this->jsRef == nullptr) {
			auto jsExports = scripts::getJSExports(env);
			auto BandcampClass = jsExports.Get("Bandcamp").As<Napi::Function>();
			auto bandcamp = BandcampClass.New({});
			auto bandcampRef = Napi::ObjectReference::New(bandcamp, 1);
			bandcampRef.SuppressDestruct();
			jsRef = bandcampRef;
		}
	}



	Promise<BandcampSearchResults> Bandcamp::search(String query, SearchOptions options) {
		return Promise<BandcampSearchResults>([=](auto resolve, auto reject) {
			queueJS([=](napi_env env) {
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
						resolve(BandcampSearchResults::fromNapiObject(info[0].As<Napi::Object>()));
					}),
					// catch
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
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
						auto obj = info[0].As<Napi::Object>();
						auto mediaType = obj.Get("type").ToString().Utf8Value();
						if(mediaType != "artist") {
							reject(BandcampError(BandcampError::Code::MEDIATYPE_MISMATCH, "Bandcamp item is " + mediaType + ", not artist"));
						} else {
							resolve(BandcampArtist::fromNapiObject(obj));
						}
					}),
					// catch
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						auto errorMessage = info[0].As<Napi::Object>().Get("message").ToString();
						reject(BandcampError(BandcampError::Code::REQUEST_FAILED, errorMessage));
					})
				});
			});
		});
	}
}

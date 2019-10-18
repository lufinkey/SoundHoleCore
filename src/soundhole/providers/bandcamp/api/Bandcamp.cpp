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
#include <embed/nodejs/NodeJS.hpp>

namespace sh {
	Bandcamp::Bandcamp()
	: jsRef(nullptr) {
		//
	}

	Bandcamp::~Bandcamp() {
		queueJSDestruct([=](napi_env env) {
			Napi::ObjectReference(env, jsRef).Unref();
		});
	}

	Napi::Object Bandcamp::getJSAPI(napi_env env) {
		if(jsRef == nullptr) {
			return Napi::Object();
		}
		return Napi::ObjectReference(env, jsRef).Value();
	}

	void Bandcamp::initializeJS(napi_env env) {
		if(this->jsRef == nullptr) {
			auto jsExports = scripts::getJSExports(env);
			auto BandcampClass = jsExports.Get("Bandcamp").As<Napi::Function>();
			auto bandcamp = BandcampClass.New({});
			jsRef = Napi::ObjectReference::New(bandcamp, 1);
		}
	}

	Promise<Json> Bandcamp::search(String query, SearchOptions options) {
		return Promise<Json>([=](auto resolve, auto reject) {
			queueJS([=](napi_env env) {
				auto jsApi = getJSAPI(env);
				if(jsApi.IsEmpty()) {
					reject(BandcampError(BandcampError::Code::NOT_INITIALIZED, "Bandcamp not initialized"));
					return;
				}
				auto jsOptions = Napi::Object::New(env);
				jsOptions.Set("page", Napi::Number::New(env, (double)options.page));
				auto promise = jsApi.Get("search").As<Napi::Function>().Call(jsApi, {
					query.toNodeJSValue(env),
					jsOptions
				}).As<Napi::Promise>();
				promise.Get("then").As<Napi::Function>().Call(promise, {
					// then
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						auto env = info.Env();
						auto jsResult = info[0].As<Napi::Object>();
						jsonPromiseFromNAPIValue(env, jsResult)
							.then(nullptr, resolve, reject);
					}),
					// catch
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						auto jsError = info[0].As<Napi::Error>();
						reject(BandcampError(BandcampError::Code::REQUEST_FAILED, jsError.Message()));
					})
				});
			});
		});
	}

	Promise<Json> Bandcamp::getTrack(String url) {
		return Promise<Json>([=](auto resolve, auto reject) {
			queueJS([=](napi_env env) {
				auto jsApi = getJSAPI(env);
				if(jsApi.IsEmpty()) {
					reject(BandcampError(BandcampError::Code::NOT_INITIALIZED, "Bandcamp not initialized"));
					return;
				}
				auto promise = jsApi.Get("getTrack").As<Napi::Function>().Call(jsApi, {
					url.toNodeJSValue(env),
				}).As<Napi::Promise>();
				promise.Get("then").As<Napi::Function>().Call(promise, {
					// then
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						auto env = info.Env();
						auto jsResult = info[0].As<Napi::Object>();
						jsonPromiseFromNAPIValue(env, jsResult)
							.then(nullptr, resolve, reject);
					}),
					// catch
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						auto jsError = info[0].As<Napi::Error>();
						reject(BandcampError(BandcampError::Code::REQUEST_FAILED, jsError.Message()));
					})
				});
			});
		});
	}

	Promise<Json> Bandcamp::getAlbum(String url) {
		return Promise<Json>([=](auto resolve, auto reject) {
			queueJS([=](napi_env env) {
				auto jsApi = getJSAPI(env);
				if(jsApi.IsEmpty()) {
					reject(BandcampError(BandcampError::Code::NOT_INITIALIZED, "Bandcamp not initialized"));
					return;
				}
				auto promise = jsApi.Get("getTrack").As<Napi::Function>().Call(jsApi, {
					url.toNodeJSValue(env),
				}).As<Napi::Promise>();
				promise.Get("then").As<Napi::Function>().Call(promise, {
					// then
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						auto env = info.Env();
						auto jsResult = info[0].As<Napi::Object>();
						jsonPromiseFromNAPIValue(env, jsResult)
							.then(nullptr, resolve, reject);
					}),
					// catch
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						auto jsError = info[0].As<Napi::Error>();
						reject(BandcampError(BandcampError::Code::REQUEST_FAILED, jsError.Message()));
					})
				});
			});
		});
	}

	Promise<Json> Bandcamp::getArtist(String url) {
		return Promise<Json>([=](auto resolve, auto reject) {
			queueJS([=](napi_env env) {
				auto jsApi = getJSAPI(env);
				if(jsApi.IsEmpty()) {
					reject(BandcampError(BandcampError::Code::NOT_INITIALIZED, "Bandcamp not initialized"));
					return;
				}
				auto promise = jsApi.Get("getTrack").As<Napi::Function>().Call(jsApi, {
					url.toNodeJSValue(env),
				}).As<Napi::Promise>();
				promise.Get("then").As<Napi::Function>().Call(promise, {
					// then
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						auto env = info.Env();
						auto jsResult = info[0].As<Napi::Object>();
						jsonPromiseFromNAPIValue(env, jsResult)
							.then(nullptr, resolve, reject);
					}),
					// catch
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						auto jsError = info[0].As<Napi::Error>();
						reject(BandcampError(BandcampError::Code::REQUEST_FAILED, jsError.Message()));
					})
				});
			});
		});
	}
}

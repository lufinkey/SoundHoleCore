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

	void Bandcamp::initializeJS(napi_env env) {
		if(this->jsRef == nullptr) {
			auto jsExports = scripts::getJSExports(env);
			auto BandcampClass = jsExports.Get("Bandcamp").As<Napi::Function>();
			auto bandcamp = BandcampClass.New({});
			jsRef = Napi::ObjectReference::New(bandcamp, 1);
		}
	}

	Promise<Bandcamp::SearchResults> Bandcamp::search(String query, SearchOptions options) {
		return Promise<Bandcamp::SearchResults>([=](auto resolve, auto reject) {
			queueJS([=](napi_env env) {
				auto jsApi = jsValue<Napi::Object>(env, jsRef);
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
						resolve(searchResultsFromNapiObject(info[0].As<Napi::Object>()));
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

	Bandcamp::SearchResults Bandcamp::searchResultsFromNapiObject(Napi::Object results) const {
		return SearchResults{
			.prevURL = stringFromNapiValue(results.Get("prevURL")),
			.nextURL = stringFromNapiValue(results.Get("nextURL")),
			.items = linkedListFromNapiArray<SearchResults::Item>(results.Get("items").template As<Napi::Array>(), [=](auto itemValue) -> SearchResults::Item {
				auto item = itemValue.template As<Napi::Object>();
				return SearchResults::Item{
					.type = ([&]() -> MediaType {
						auto mediaType = stringFromNapiValue(item.Get("type"));
						if(mediaType == "track") {
							return MediaType::TRACK;
						} else if(mediaType == "artist") {
							return MediaType::ARTIST;
						} else if(mediaType == "album") {
							return MediaType::ALBUM;
						} else if(mediaType == "label") {
							return MediaType::LABEL;
						} else if(mediaType == "fan") {
							return MediaType::FAN;
						} else {
							return MediaType::UNKNOWN;
						}
					})(),
					.name = stringFromNapiValue(item.Get("name")),
					.url = stringFromNapiValue(item.Get("url")),
					.imageURL = stringFromNapiValue(item.Get("imageURL")),
					.tags = arrayListFromNapiArray<String>(item.Get("tags").template As<Napi::Array>(), [](auto tag) -> String {
						return tag.ToString();
					}),
					.genre = stringFromNapiValue(item.Get("genre")),
					.releaseDate = stringFromNapiValue(item.Get("releaseDate")),
					.artistName = stringFromNapiValue(item.Get("artistName")),
					.artistURL = stringFromNapiValue(item.Get("artistURL")),
					.albumName = stringFromNapiValue(item.Get("albumName")),
					.albumURL = stringFromNapiValue(item.Get("albumURL")),
					.location = stringFromNapiValue(item.Get("location")),
					.numTracks = ([&]() -> Optional<size_t> {
						auto numTracks = item.Get("numTracks").template As<Napi::Number>();
						if(numTracks.IsEmpty() || numTracks.IsNull() || numTracks.IsUndefined()) {
							return std::nullopt;
						}
						return Optional<size_t>(numTracks.Int64Value());
					})(),
					.numMinutes = ([&]() -> Optional<size_t> {
						auto numMinutes = item.Get("numMinutes").template As<Napi::Number>();
						if(numMinutes.IsEmpty() || numMinutes.IsNull() || numMinutes.IsUndefined()) {
							return std::nullopt;
						}
						return Optional<size_t>(numMinutes.Int64Value());
					})()
				};
			})
		};
	}

	Promise<Json> Bandcamp::getTrack(String url) {
		return Promise<Json>([=](auto resolve, auto reject) {
			queueJS([=](napi_env env) {
				auto jsApi = jsValue<Napi::Object>(env, jsRef);
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
						resolve(jsonFromNapiValue(info[0].As<Napi::Object>()));
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

	Promise<Json> Bandcamp::getAlbum(String url) {
		return Promise<Json>([=](auto resolve, auto reject) {
			queueJS([=](napi_env env) {
				auto jsApi = jsValue<Napi::Object>(env, jsRef);
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
						resolve(jsonFromNapiValue(info[0].As<Napi::Object>()));
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

	Promise<Json> Bandcamp::getArtist(String url) {
		return Promise<Json>([=](auto resolve, auto reject) {
			queueJS([=](napi_env env) {
				auto jsApi = jsValue<Napi::Object>(env, jsRef);
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
						resolve(jsonFromNapiValue(info[0].As<Napi::Object>()));
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

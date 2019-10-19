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
						auto json = jsonFromNapiValue(info[0].As<Napi::Object>());
						getDefaultPromiseQueue()->async([=]() {
							resolve(searchResultsFromJson(json));
						});
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

	Bandcamp::SearchResults Bandcamp::searchResultsFromJson(Json json) const {
		return SearchResults{
			.prevURL = json["prevURL"].string_value(),
			.nextURL = json["nextURL"].string_value(),
			.items = ArrayList<Json>(json["items"].array_items()).map<SearchResults::Item>([=](auto item) -> SearchResults::Item {
				return SearchResults::Item{
					.type = ([&]() -> MediaType {
						auto mediaType = item["type"].string_value();
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
					.name = item["name"].string_value(),
					.url = item["url"].string_value(),
					.imageURL = item["imageURL"].string_value(),
					.tags = ArrayList<Json>(item["tags"].array_items()).map<String>([](auto tag) -> String {
						return tag.string_value();
					}),
					.genre = item["genre"].string_value(),
					.releaseDate = item["releaseDate"].string_value(),
					.artistName = item["artistName"].string_value(),
					.artistURL = item["artistURL"].string_value(),
					.albumName = item["albumName"].string_value(),
					.albumURL = item["albumURL"].string_value(),
					.location = item["location"].string_value(),
					.numTracks = ([&]() -> Optional<size_t> {
						auto numTracks = item["numTracks"];
						return (!numTracks.is_null()) ? Optional<size_t>((size_t)numTracks.int_value()) : std::nullopt;
					})(),
					.numMinutes = ([&]() -> Optional<size_t> {
						auto numMinutes = item["numMinutes"];
						return (!numMinutes.is_null()) ? Optional<size_t>((size_t)numMinutes.int_value()) : std::nullopt;
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

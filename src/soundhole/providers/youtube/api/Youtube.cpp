//
//  Youtube.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include <napi.h>
#include "Youtube.hpp"
#include "YoutubeError.hpp"
#include <soundhole/scripts/Scripts.hpp>

namespace sh {
	String YOUTUBE_API_URL = "https://www.googleapis.com/youtube/v3";

	Youtube::Youtube(Options options)
	: options(options), jsRef(nullptr) {
		//
	}

	Youtube::~Youtube() {
		queueJSDestruct([=](napi_env env) {
			Napi::ObjectReference(env, jsRef).Unref();
		});
	}

	void Youtube::initializeJS(napi_env env) {
		if(this->jsRef == nullptr) {
			auto jsExports = scripts::getJSExports(env);
			auto ytdl = jsExports.Get("ytdl").As<Napi::Object>();
			jsRef = Napi::ObjectReference::New(ytdl, 1);
		}
	}

	Promise<Json> Youtube::sendApiRequest(utils::HttpMethod method, String endpoint, std::map<String,String> query, Json body) {
		query["key"] = options.apiKey;
		auto request = utils::HttpRequest{
			.url = Url(YOUTUBE_API_URL+'/'+endpoint+'?'+utils::makeQueryString(query)),
			.method = method,
			.headers = {
				{ "Content-Type", "application/json" }
			},
			.data = ((!body.is_null()) ? body.dump() : "")
		};
		return utils::performHttpRequest(request).map<Json>([=](auto response) -> Json {
			std::string parseError;
			auto responseJson = (response->data.length() > 0) ? Json::parse((std::string)response->data, parseError) : Json();
			if(response->statusCode >= 300) {
				if(parseError.empty()) {
					auto errorMessage = responseJson["error"]["message"].string_value();
					if(errorMessage.empty()) {
						errorMessage = (std::string)response->statusMessage;
					}
					throw YoutubeError(YoutubeError::Code::REQUEST_FAILED, errorMessage);
				} else {
					throw YoutubeError(YoutubeError::Code::REQUEST_FAILED, response->statusMessage);
				}
			}
			return responseJson;
		});
	}

	Promise<Json> Youtube::search(String query, SearchOptions options) {
		auto params = std::map<String,String>();
		params["q"] = query;
		params["part"] = "id,snippet";
		if(options.types.size() > 0) {
			params["type"] = String::join(options.types.map<String>([](auto& type) -> String {
				switch(type) {
					case MediaType::VIDEO:
						return "video";
					case MediaType::CHANNEL:
						return "channel";
					case MediaType::PLAYLIST:
						return "playlist";
					default:
						throw std::runtime_error("invalid MediaType value");
				}
			}), ",");
		}
		return sendApiRequest(utils::HttpMethod::GET, "search", params, nullptr);
	}

	Promise<Json> Youtube::getVideoInfo(String id) {
		String url = "https://www.youtube.com/watch?v=" + id;
		return Promise<Json>([=](auto resolve, auto reject) {
			queueJS([=](napi_env env) {
				auto jsApi = jsValue<Napi::Object>(env, jsRef);
				if(jsApi.IsEmpty()) {
					reject(YoutubeError(YoutubeError::Code::NOT_INITIALIZED, "ytdl not initialized"));
					return;
				}
				jsApi.Get("getInfo").As<Napi::Function>().Call(jsApi, {
					url.toNodeJSValue(env),
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						auto error = info[0].As<Napi::Object>();
						auto result = info[1].As<Napi::Object>();
						if(!error.IsNull() && !error.IsUndefined()) {
							auto errorMessage = error.Get("message").ToString();
							reject(YoutubeError(YoutubeError::Code::REQUEST_FAILED, errorMessage));
						} else {
							resolve(jsonFromNapiValue(result));
						}
					})
				});
			});
		});
	}

	Promise<Json> Youtube::getVideo(String id) {
		return sendApiRequest(utils::HttpMethod::GET, "videos", {
			{ "id", id },
			{ "part", "id,snippet" }
		}, nullptr);
	}

	Promise<Json> Youtube::getChannel(String id) {
		return sendApiRequest(utils::HttpMethod::GET, "channels", {
			{ "id", id },
			{ "part", "id,snippet" }
		}, nullptr);
	}

	Promise<Json> Youtube::getPlaylist(String id) {
		return sendApiRequest(utils::HttpMethod::GET, "playlists", {
			{ "id", id },
			{ "part", "id,snippet" }
		}, nullptr);
	}
}

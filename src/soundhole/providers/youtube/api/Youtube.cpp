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

	String Youtube::MediaType_toString(MediaType mediaType) {
		switch(mediaType) {
			case MediaType::VIDEO:
				return "video";
			case MediaType::CHANNEL:
				return "channel";
			case MediaType::PLAYLIST:
				return "playlist";
		}
		throw std::runtime_error("invalid MediaType value");
	}


	Youtube::Youtube(Options options)
	: options(options), jsRef(nullptr) {
		//
	}

	Youtube::~Youtube() {
		queueJSDestruct([=](napi_env env) {
			auto napiRef = Napi::ObjectReference(env, jsRef);
			if(napiRef.Unref() == 0) {
				napiRef.Reset();
			} else {
				napiRef.SuppressDestruct();
			}
		});
	}

	void Youtube::initializeJS(napi_env env) {
		if(this->jsRef == nullptr) {
			auto jsExports = scripts::getJSExports(env);
			auto ytdl = jsExports.Get("ytdl").As<Napi::Object>();
			auto ytdlRef = Napi::ObjectReference::New(ytdl, 1);
			ytdlRef.SuppressDestruct();
			jsRef = ytdlRef;
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

	Promise<YoutubePage<YoutubeSearchResult>> Youtube::search(String query, SearchOptions options) {
		auto params = std::map<String,String>{
			{ "q", query },
			{ "part", "id,snippet" }
		};
		if(options.types.size() > 0) {
			params["type"] = String::join(options.types.map<String>([](auto& type) -> String {
				return MediaType_toString(type);
			}), ",");
		}
		if(!options.pageToken.empty()) {
			params["pageToken"] = options.pageToken;
		}
		return sendApiRequest(utils::HttpMethod::GET, "search", params, nullptr)
		.map<YoutubePage<YoutubeSearchResult>>([](auto json) {
			return YoutubePage<YoutubeSearchResult>::fromJson(json);
		});
	}

	Promise<YoutubeVideoInfo> Youtube::getVideoInfo(String id) {
		String url = "https://www.youtube.com/watch?v=" + id;
		return Promise<YoutubeVideoInfo>([=](auto resolve, auto reject) {
			queueJS([=](napi_env env) {
				auto jsApi = jsutils::jsValue<Napi::Object>(env, jsRef);
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
							resolve(YoutubeVideoInfo::fromNapiObject(result));
						}
					})
				});
			});
		});
	}

	Promise<YoutubeVideo> Youtube::getVideo(String id) {
		return sendApiRequest(utils::HttpMethod::GET, "videos", {
			{ "id", id },
			{ "part", "id,snippet" }
		}, nullptr).map<YoutubeVideo>([](auto json) {
			return YoutubeVideo::fromJson(json);
		});
	}

	Promise<YoutubeChannel> Youtube::getChannel(String id) {
		return sendApiRequest(utils::HttpMethod::GET, "channels", {
			{ "id", id },
			{ "part", "id,snippet" }
		}, nullptr).map<YoutubeChannel>([](auto json) {
			return YoutubeChannel::fromJson(json);
		});
	}

	Promise<YoutubePlaylist> Youtube::getPlaylist(String id) {
		return sendApiRequest(utils::HttpMethod::GET, "playlists", {
			{ "id", id },
			{ "part", "id,snippet" }
		}, nullptr).map<YoutubePlaylist>([](auto json) {
			return YoutubePlaylist::fromJson(json);
		});
	}

	Promise<YoutubePage<YoutubePlaylistItem>> Youtube::getPlaylistItems(String id, GetPlaylistItemsOptions options) {
		auto query = std::map<String,String>{
			{ "playlistId", id },
			{ "part", "id,snippet" }
		};
		if(options.maxResults.has_value()) {
			query["maxResults"] = std::to_string(options.maxResults.value());
		}
		if(!options.pageToken.empty()) {
			query["pageToken"] = options.pageToken;
		}
		if(!options.videoId.empty()) {
			query["videoId"] = options.videoId;
		}
		return sendApiRequest(utils::HttpMethod::GET, "playlistItems", query, nullptr).map<YoutubePage<YoutubePlaylistItem>>([](auto json) {
			return YoutubePage<YoutubePlaylistItem>::fromJson(json);
		});
	}
}

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
#include <soundhole/utils/js/JSWrapClass.impl.hpp>

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
		throw std::invalid_argument("invalid Youtube::MediaType value");
	}

	String Youtube::ResultPart_toString(ResultPart part) {
		switch(part) {
			case ResultPart::ID:
				return "id";
			case ResultPart::SNIPPET:
				return "snippet";
			case ResultPart::CONTENT_DETAILS:
				return "contentDetails";
		}
		throw std::invalid_argument("invalid Youtube::ResultPart value");
	}

	String Youtube::SearchOrder_toString(SearchOrder order) {
		switch(order) {
			case SearchOrder::RELEVANCE:
				return "relevance";
			case SearchOrder::DATE:
				return "date";
			case SearchOrder::RATING:
				return "rating";
			case SearchOrder::TITLE:
				return "title";
			case SearchOrder::VIDEO_COUNT:
				return "videoCount";
			case SearchOrder::VIEW_COUNT:
				return "viewCount";
		}
		throw std::invalid_argument("invalid Youtube::SearchOrder value");
	}



	Youtube::Youtube(Options options)
	: jsRef(nullptr), auth(new YoutubeAuth(options.auth)) {
		auth->load();
	}

	Youtube::~Youtube() {
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

	void Youtube::initializeJS(napi_env env) {
		if(this->jsRef == nullptr) {
			auto jsExports = scripts::getJSExports(env);
			auto ytdl = jsExports.Get("ytdl").As<Napi::Object>();
			auto ytdlRef = Napi::ObjectReference::New(ytdl, 1);
			ytdlRef.SuppressDestruct();
			jsRef = ytdlRef;
		}
	}




	YoutubeAuth* Youtube::getAuth() {
		return auth;
	}
	const YoutubeAuth* Youtube::getAuth() const {
		return auth;
	}

	Promise<bool> Youtube::login() {
		return auth->login();
	}
	void Youtube::logout() {
		auth->logout();
	}
	bool Youtube::isLoggedIn() const {
		return auth->isLoggedIn();
	}




	Promise<Json> Youtube::sendApiRequest(utils::HttpMethod method, String endpoint, std::map<String,String> query, Json body) {
		auto session = auth->getSession();
		auto& apiKey = auth->getOptions().apiKey;
		if(!session && !apiKey.empty()) {
			query["key"] = apiKey;
		}
		utils::HttpHeaders headers;
		String bodyData = ((!body.is_null()) ? body.dump() : String());
		if(!body.is_null()) {
			headers.set("Content-Type", "application/json");
			headers.set("Content-Length", std::to_string(bodyData.length()));
		}
		if(session) {
			headers.set("Authorization", session->getTokenType()+" "+session->getAccessToken());
		}
		auto request = utils::HttpRequest{
			.url = URL(YOUTUBE_API_URL+'/'+endpoint+'?'+URL::makeQueryString(query)),
			.method = method,
			.headers = headers,
			.data = bodyData
		};
		auto response = co_await utils::performHttpRequest(request);
		// if response is unauthorized, refresh token and try again
		if(response->statusCode == 401) {
			session = auth->getSession();
			if(session && !session->getRefreshToken().empty()) {
				co_await auth->renewSession();
				response = co_await utils::performHttpRequest(request);
			}
		}
		// parse response json
		std::string parseError;
		auto responseJson = (response->data.length() > 0) ? Json::parse(response->data, parseError) : Json();
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
		co_return responseJson;
	}




	Promise<YoutubePage<YoutubeSearchResult>> Youtube::search(String searchQuery, SearchOptions options) {
		auto query = std::map<String,String>{
			{ "part", "id,snippet" }
		};
		if(!searchQuery.empty()) {
			query["q"] = searchQuery;
		}
		if(options.types.size() > 0) {
			query["type"] = String::join(options.types.map([](auto& type) -> String {
				return MediaType_toString(type);
			}), ",");
		}
		if(options.maxResults.has_value()) {
			query["maxResults"] = std::to_string(options.maxResults.value());
		}
		if(!options.pageToken.empty()) {
			query["pageToken"] = options.pageToken;
		}
		if(!options.channelId.empty()) {
			query["channelId"] = options.channelId;
		}
		if(options.order.has_value()) {
			query["order"] = SearchOrder_toString(options.order.value());
		}
		return sendApiRequest(utils::HttpMethod::GET, "search", query, nullptr)
		.map([](auto json) -> YoutubePage<YoutubeSearchResult> {
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
				auto promise = jsApi.Get("getInfo").As<Napi::Function>().Call(jsApi, { url.toNodeJSValue(env) }).As<Napi::Promise>();
				promise.Get("then").As<Napi::Function>().Call(promise, {
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						auto result = info[0].As<Napi::Object>();
						resolve(YoutubeVideoInfo::fromNapiObject(result));
					}),
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						auto error = info[0].As<Napi::Object>();
						auto errorMessage = error.Get("message").ToString();
						reject(YoutubeError(YoutubeError::Code::REQUEST_FAILED, errorMessage));
					})
				});
			});
		});
	}

	Promise<YoutubeVideo> Youtube::getVideo(String id) {
		return sendApiRequest(utils::HttpMethod::GET, "videos", {
			{ "id", id },
			{ "part", "id,snippet,contentDetails" }
		}, nullptr).map([=](auto json) -> YoutubeVideo {
			auto page = YoutubePage<YoutubeVideo>::fromJson(json);
			if(page.items.size() == 0) {
				throw YoutubeError(YoutubeError::Code::NOT_FOUND, "Could not find video with id "+id);
			}
			return page.items.front();
		});
	}

	Promise<ArrayList<YoutubeVideo>> Youtube::getVideos(ArrayList<String> ids) {
		return sendApiRequest(utils::HttpMethod::GET, "videos", {
			{ "id", String::join(ids, ",") },
			{ "part", "id,snippet,contentDetails" }
		}, nullptr).map([=](auto json) -> ArrayList<YoutubeVideo> {
			return YoutubePage<YoutubeVideo>::fromJson(json).items;
		});
	}

	Promise<void> Youtube::rateVideo(String videoId, YoutubeVideo::Rating rating) {
		return sendApiRequest(utils::HttpMethod::POST, "videos/rate", {
			{ "id", videoId },
			{ "rating", YoutubeVideo::Rating_toString(rating) }
		}, nullptr).toVoid();
	}




	Promise<YoutubePage<YoutubeChannel>> Youtube::getChannels(GetChannelsOptions options) {
		auto query = std::map<String,String>{
			{ "part", "id,snippet,contentDetails,status,statistics,topicDetails" }
		};
		if(!options.ids.empty()) {
			query["id"] = String::join(options.ids,",");
		}
		if(!options.forUsername.empty()) {
			query["forUsername"] = options.forUsername;
		}
		if(options.managedByMe) {
			query["managedByMe"] = options.managedByMe.value() ? "true" : "false";
		}
		if(options.mine) {
			query["mine"] = options.mine.value() ? "true" : "false";
		}
		if(options.maxResults) {
			query["maxResults"] = std::to_string(options.maxResults.value());
		}
		if(!options.pageToken.empty()) {
			query["pageToken"] = options.pageToken;
		}
		return sendApiRequest(utils::HttpMethod::GET, "channels", query, nullptr).map([=](auto json) -> YoutubePage<YoutubeChannel> {
			return YoutubePage<YoutubeChannel>::fromJson(json);
		});
	}

	Promise<YoutubeChannel> Youtube::getChannel(String id) {
		return getChannels({
			.ids = { id }
		}).map([=](auto page) -> YoutubeChannel {
			if(page.items.size() == 0) {
				throw YoutubeError(YoutubeError::Code::NOT_FOUND, "Could not find channel with id "+id);
			}
			return page.items.front();
		});
	}

	Promise<YoutubePage<YoutubeChannel>> Youtube::getMyChannels(GetMyChannelsOptions options) {
		return getChannels({
			.mine=true,
			.maxResults=options.maxResults,
			.pageToken=options.pageToken
		});
	}



	Promise<YoutubePage<YoutubePlaylist>> Youtube::getChannelPlaylists(String id, GetChannelPlaylistsOptions options) {
		return getPlaylists({
			.channelId = id,
			.maxResults = options.maxResults,
			.pageToken = options.pageToken
		});
	}




	Promise<YoutubeItemList<YoutubeChannelSection>> Youtube::getChannelSections(GetChannelSectionsOptions options) {
		auto query = std::map<String,String>{
			{ "part", "id,snippet,contentDetails,localizations,targeting" }
		};
		if(options.ids.size() > 0) {
			query["id"] = String::join(options.ids,",");
		}
		if(!options.channelId.empty()) {
			query["channelId"] = options.channelId;
		}
		if(options.mine.hasValue()) {
			query["mine"] = options.mine.value() ? "true" : "false";
		}
		return sendApiRequest(utils::HttpMethod::GET, "channelSections", query, nullptr).map([=](auto json) -> YoutubeItemList<YoutubeChannelSection> {
			return YoutubeItemList<YoutubeChannelSection>::fromJson(json);
		});
	}

	Promise<YoutubeChannelSection> Youtube::insertChannelSection(InsertChannelSectionOptions options) {
		auto query = std::map<String,String>{
			{ "part", "id,snippet,contentDetails,localizations,targeting" }
		};
		auto body = Json::object{};
		auto snippet = Json::object{};
		if(!options.type.empty()) {
			snippet["type"] = (std::string)options.type;
		}
		if(!options.style.empty()) {
			snippet["style"] = (std::string)options.style;
		}
		if(options.title) {
			snippet["title"] = (std::string)options.title.value();
		}
		if(options.position) {
			snippet["position"] = (int)options.position.value();
		}
		if(!options.defaultLanguage.empty()) {
			snippet["defaultLanguage"] = (std::string)options.defaultLanguage;
		}
		body["snippet"] = snippet;
		if(!options.playlists.empty() || !options.channels.empty() || options.type == "singlePlaylist" || options.type == "multiplePlaylists" || options.type == "singleChannel" || options.type == "multipleChannels") {
			auto contentDetails = Json::object{};
			if(!options.playlists.empty() || options.type == "singlePlaylist" || options.type == "multiplePlaylists") {
				contentDetails["playlists"] = options.playlists.map([](auto playlistId) -> Json {
					return Json((std::string)playlistId);
				});
			}
			if(!options.channels.empty() || options.type == "singleChannel" || options.type == "multipleChannels") {
				contentDetails["channels"] = options.channels.map([](auto channelId) -> Json {
					return Json((std::string)channelId);
				});
			}
			body["contentDetails"] = contentDetails;
		}
		if(!options.localizations.empty()) {
			auto localizations = Json::object{};
			for(auto& pair : options.localizations) {
				localizations[pair.first] = pair.second.toJson();
			}
			body["localizations"] = localizations;
		}
		if(options.targeting) {
			body["targeting"] = Json::object{
				{ "languages", options.targeting->languages.map([](auto language) -> Json {
					return Json((std::string)language);
				}) },
				{ "regions", options.targeting->regions.map([](auto region) -> Json {
					return Json((std::string)region);
				}) },
				{ "countries", options.targeting->countries.map([](auto country) -> Json {
					return Json((std::string)country);
				}) }
			};
		}
		return sendApiRequest(utils::HttpMethod::POST, "channelSections", query, body)
		.map([=](auto json) -> YoutubeChannelSection {
			return YoutubeChannelSection::fromJson(json);
		});
	}

	Promise<YoutubeChannelSection> Youtube::updateChannelSection(String channelSectionId, UpdateChannelSectionOptions options) {
		auto parts = LinkedList<String>{ "id", "player" };
		auto body = Json::object{
			{ "id", (std::string)channelSectionId }
		};
		if(!options.type.empty() || !options.style.empty() || options.title || options.position || options.defaultLanguage) {
			auto snippet = Json::object{};
			if(!options.type.empty()) {
				snippet["type"] = (std::string)options.type;
			}
			if(!options.style.empty()) {
				snippet["style"] = (std::string)options.style;
			}
			if(options.title) {
				snippet["title"] = (std::string)options.title.value();
			}
			if(options.position) {
				snippet["position"] = (int)options.position.value();
			}
			if(options.defaultLanguage) {
				snippet["defaultLanguage"] = (std::string)options.defaultLanguage.value();
			}
			body["snippet"] = snippet;
		}
		if(options.playlists || options.channels) {
			auto contentDetails = Json::object{};
			if(options.playlists) {
				contentDetails["playlists"] = options.playlists->map([](auto playlistId) -> Json {
					return Json((std::string)playlistId);
				});
			}
			if(options.channels) {
				contentDetails["channels"] = options.channels->map([](auto channelId) -> Json {
					return Json((std::string)channelId);
				});
			}
			body["contentDetails"] = contentDetails;
		}
		if(options.targeting) {
			body["targeting"] = Json::object{
				{ "languages", options.targeting->languages.map([](auto language) -> Json {
					return Json((std::string)language);
				}) },
				{ "regions", options.targeting->regions.map([](auto region) -> Json {
					return Json((std::string)region);
				}) },
				{ "countries", options.targeting->countries.map([](auto country) -> Json {
					return Json((std::string)country);
				}) }
			};
		}
		auto query = std::map<String,String>{
			{ "part", String::join(parts,",") }
		};
		return sendApiRequest(utils::HttpMethod::PUT, "channelSections", query, body).map([](auto json) -> YoutubeChannelSection {
			return YoutubeChannelSection::fromJson(json);
		});
	}

	Promise<void> Youtube::deleteChannelSection(String channelSectionId) {
		auto query = std::map<String,String>{
			{ "id", channelSectionId }
		};
		return sendApiRequest(utils::HttpMethod::DELETE, "channelSections", query, nullptr).toVoid();
	}




	Promise<YoutubePage<YoutubePlaylist>> Youtube::getPlaylists(GetPlaylistsOptions options) {
		auto query = std::map<String,String>{
			{ "part", "id,snippet,status,contentDetails,player,localizations" }
		};
		if(options.ids.size() > 0) {
			query["id"] = String::join(options.ids,",");
		}
		if(!options.channelId.empty()) {
			query["channelId"] = options.channelId;
		}
		if(options.mine.hasValue()) {
			query["mine"] = options.mine.value() ? "true" : "false";
		}
		if(options.maxResults.hasValue()) {
			query["maxResults"] = std::to_string(options.maxResults.value());
		}
		if(!options.pageToken.empty()) {
			query["pageToken"] = options.pageToken;
		}
		return sendApiRequest(utils::HttpMethod::GET, "playlists", query, nullptr).map([](auto json) -> YoutubePage<YoutubePlaylist> {
			return YoutubePage<YoutubePlaylist>::fromJson(json);
		});
	}

	Promise<YoutubePlaylist> Youtube::getPlaylist(String id) {
		return getPlaylists({
			.ids = { id }
		}).map([=](auto page) -> YoutubePlaylist {
			if(page.items.size() == 0) {
				throw YoutubeError(YoutubeError::Code::NOT_FOUND, "Could not find playlist with id "+id);
			}
			return page.items.front();
		});
	}

	Promise<YoutubePage<YoutubePlaylist>> Youtube::getMyPlaylists(GetMyPlaylistsOptions options) {
		return getPlaylists({
			.mine=true,
			.maxResults=options.maxResults,
			.pageToken=options.pageToken
		});
	}

	Promise<YoutubePlaylist> Youtube::createPlaylist(String title, CreatePlaylistOptions options) {
		auto query = std::map<String,String>{
			{ "part", "id,snippet,status,contentDetails,player" }
		};
		auto body = Json::object{};
		auto snippet = Json::object{
			{ "title", (std::string)title },
			{ "description", (std::string)options.description },
			{ "tags", options.tags.map([](auto& tag) -> Json {
				return Json((std::string)tag);
			}) }
		};
		if(!options.privacyStatus.empty()) {
			body["status"] = Json::object{
				{ "privacyStatus", (std::string)options.privacyStatus }
			};
		}
		if(!options.defaultLanguage.empty()) {
			snippet["defaultLanguage"] = (std::string)options.defaultLanguage;
		}
		if(!options.localizations.empty()) {
			auto localizations = Json::object{};
			for(auto& pair : options.localizations) {
				localizations[pair.first] = pair.second.toJson();
			}
			body["localizations"] = localizations;
		}
		body["snippet"] = snippet;
		return sendApiRequest(utils::HttpMethod::POST, "playlists", query, body).map([](auto json) -> YoutubePlaylist {
			return YoutubePlaylist::fromJson(json);
		});
	}

	Promise<YoutubePlaylist> Youtube::updatePlaylist(String playlistId, UpdatePlaylistOptions options) {
		auto parts = LinkedList<String>{ "id", "player" };
		auto body = Json::object{
			{ "id", (std::string)playlistId }
		};
		if(options.title || options.description || options.tags || options.defaultLanguage) {
			parts.pushBack("snippet");
			auto snippet = Json::object{};
			if(options.title) {
				snippet["title"] = (std::string)options.title.value();
			}
			if(options.description) {
				snippet["description"] = (std::string)options.description.value();
			}
			if(options.tags) {
				snippet["tags"] = options.tags->map([](auto& tag) -> Json {
					return Json((std::string)tag);
				});
			}
			if(options.defaultLanguage) {
				if(options.defaultLanguage->empty()) {
					snippet["defaultLanguage"] = Json();
				} else {
					snippet["defaultLanguage"] = (std::string)options.defaultLanguage.value();
				}
			}
			body["snippet"] = snippet;
		}
		if(options.privacyStatus) {
			parts.pushBack("status");
			auto status = Json::object{};
			if(options.privacyStatus->empty()) {
				status["privacyStatus"] = Json();
			} else {
				status["privacyStatus"] = (std::string)options.privacyStatus.value();
			}
			body["status"] = status;
		}
		if(options.localizations) {
			parts.pushBack("localizations");
			auto localizations = Json::object{};
			for(auto& pair : options.localizations.value()) {
				localizations[pair.first] = pair.second.toJson();
			}
			body["localizations"] = localizations;
		}
		auto query = std::map<String,String>{
			{ "part", String::join(parts,",") }
		};
		return sendApiRequest(utils::HttpMethod::PUT, "playlists", query, body).map([](auto json) -> YoutubePlaylist {
			return YoutubePlaylist::fromJson(json);
		});
	}

	Promise<void> Youtube::deletePlaylist(String playlistId) {
		auto query = std::map<String,String>{
			{ "id", playlistId }
		};
		return sendApiRequest(utils::HttpMethod::DELETE, "playlists", query, Json()).toVoid();
	}




	Promise<YoutubePage<YoutubePlaylistItem>> Youtube::getPlaylistItems(String playlistId, GetPlaylistItemsOptions options) {
		auto query = std::map<String,String>{
			{ "playlistId", playlistId },
			{ "part", "id,snippet,contentDetails,status" }
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
		return sendApiRequest(utils::HttpMethod::GET, "playlistItems", query, nullptr).map([](auto json) -> YoutubePage<YoutubePlaylistItem> {
			return YoutubePage<YoutubePlaylistItem>::fromJson(json);
		});
	}

	Promise<YoutubePlaylistItem> Youtube::insertPlaylistItem(String playlistId, String resourceId, InsertPlaylistItemOptions options) {
		auto query = std::map<String,String>{
			{ "part", "id,snippet,contentDetails,status" }
		};
		auto snippet = Json::object{
			{ "playlistId", (std::string)playlistId },
			{ "resourceId", (std::string)resourceId },
		};
		auto contentDetails = Json::object{};
		if(options.position) {
			snippet["position"] = (int)options.position.value();
		}
		if(!options.note.empty()) {
			contentDetails["note"] = (std::string)options.note;
		}
		if(!options.startAt.empty()) {
			contentDetails["startAt"] = (std::string)options.startAt;
		}
		if(!options.endAt.empty()) {
			contentDetails["endAt"] = (std::string)options.endAt;
		}
		auto body = Json::object{
			{ "snippet", snippet },
			{ "contentDetails", contentDetails }
		};
		return sendApiRequest(utils::HttpMethod::POST, "playlistItems", query, body).map([](auto json) -> YoutubePlaylistItem {
			return YoutubePlaylistItem::fromJson(json);
		});
	}
	
	Promise<YoutubePlaylistItem> Youtube::updatePlaylistItem(String playlistItemId, UpdatePlaylistItemOptions options) {
		auto parts = LinkedList<String>{ "id" };
		auto body = Json::object{
			{ "id", (std::string)playlistItemId }
		};
		if(!options.playlistId.empty() || !options.resourceId.empty() || options.position) {
			parts.pushBack("snippet");
			auto snippet = Json::object{};
			if(!options.playlistId.empty()) {
				snippet["playlistId"] = (std::string)options.playlistId;
			}
			if(!options.resourceId.empty()) {
				snippet["resourceId"] = (std::string)options.resourceId;
			}
			if(options.position) {
				snippet["position"] = (int)options.position.value();
			}
		}
		if(options.note || options.startAt || options.endAt) {
			parts.pushBack("contentDetails");
			auto contentDetails = Json::object{};
			if(options.note) {
				contentDetails["note"] = (std::string)options.note.value();
			}
			if(!options.startAt) {
				if(options.startAt->empty()) {
					contentDetails["startAt"] = Json();
				} else {
					contentDetails["startAt"] = (std::string)options.startAt.value();
				}
			}
			if(!options.endAt) {
				if(options.endAt->empty()) {
					contentDetails["endAt"] = Json();
				} else {
					contentDetails["endAt"] = (std::string)options.endAt.value();
				}
			}
			body["contentDetails"] = contentDetails;
		}
		auto query = std::map<String,String>{
			{ "part", String::join(parts,",") }
		};
		return sendApiRequest(utils::HttpMethod::PUT, "playlistItems", query, body).map([](auto json) -> YoutubePlaylistItem {
			return YoutubePlaylistItem::fromJson(json);
		});
	}
	
	Promise<void> Youtube::deletePlaylistItem(String playlistItemId) {
		auto query = std::map<String,String>{
			{ "id", playlistItemId }
		};
		return sendApiRequest(utils::HttpMethod::DELETE, "playlistItems", query, nullptr).toVoid();
	}
}

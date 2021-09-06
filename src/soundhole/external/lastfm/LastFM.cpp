//
//  LastFM.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/5/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "LastFM.hpp"
#include "LastFMError.hpp"
#include <soundhole/utils/SecureStore.hpp>

namespace sh {
	LastFM::LastFM(Options options): options(options), _session(loadSession()) {
		//
	}

	Optional<LastFMSession> LastFM::loadSession() {
		if(options.sessionPersistKey.empty()) {
			return std::nullopt;
		}
		auto sessionData = SecureStore::getSecureData(options.sessionPersistKey).toString();
		std::string parseError;
		auto sessionJson = Json::parse(sessionData, parseError);
		return sessionJson.is_null() ? std::nullopt : maybeTry([&]() { return LastFMSession::fromJson(sessionJson); });
	}

	void LastFM::saveSession() {
		if(options.sessionPersistKey.empty()) {
			return;
		}
		if(_session) {
			SecureStore::setSecureData(options.sessionPersistKey, Data(_session->toJson().dump()));
		} else {
			SecureStore::deleteSecureData(options.sessionPersistKey);
		}
	}


	Promise<Json> LastFM::sendRequest(utils::HttpMethod httpMethod, String apiMethod, std::map<String,String> params, bool usesAuth) {
		String url = "https://ws.audioscrobbler.com/2.0";
		params["method"] = apiMethod;
		params["format"] = "json";
		params["api_key"] = options.apiKey;
		if(usesAuth && _session) {
			params["sk"] = _session->key;
		}
		{ // create signature
			String requestSig;
			auto ignoredParams = ArrayList<String>{ "api_sig", "format" };
			for(auto& pair : params) {
				if(ignoredParams.contains(pair.first)) {
					continue;
				}
				requestSig += pair.first;
				requestSig += pair.second;
			}
			requestSig += options.apiSecret;
			requestSig = md5(requestSig);
			params["api_sig"] = requestSig;
		}
		
		// create body
		String body;
		if(httpMethod == utils::HttpMethod::GET) {
			if(params.size() > 0) {
				url += "?";
				url += utils::makeQueryString(params);
			}
		} else {
			if(params.size() > 0) {
				body = utils::makeQueryString(params);
			}
		}
		
		// add headers
		std::map<String,String> headers;
		if(body.length() > 0) {
			headers["Content-Type"] = "application/x-www-form-urlencoded";
			headers["Content-Length"] = std::to_string(body.length());
		}
		
		// build request
		auto request = utils::HttpRequest{
			.url = Url(url),
			.method = httpMethod,
			.headers = headers,
			.data = body
		};
		// perform request
		return utils::performHttpRequest(request).map(nullptr, [](utils::SharedHttpResponse response) {
			// map response
			std::string parseError;
			auto json = Json::parse(response->data, parseError);
			auto errorJson = json["error"];
			if(response->statusCode >= 300 || errorJson.number_value() != 0) {
				// handle error
				auto messageJson = json["message"];
				throw LastFMError(LastFMError::Code::REQUEST_FAILED,
					messageJson.is_string() ?
						messageJson.string_value()
						: "Request failed with error "+std::to_string(errorJson.number_value()));
			}
			if(!parseError.empty()) {
				throw LastFMError(LastFMError::Code::BAD_DATA, "failed to parse response: "+parseError);
			}
			return json;
		});
	}


	Promise<LastFMSession> LastFM::authenticate(String username, String password) {
		return sendRequest(utils::HttpMethod::POST, "auth.getMobileSession", {
			{ "username", username },
			{ "password", password }
		}, false).map(nullptr, [](Json response) {
			return LastFMSession::fromJson(response["session"]);
		});
	}

	Promise<void> LastFM::login(String username, String password) {
		return authenticate(username, password).then([=](auto session) {
			this->_session = session;
			saveSession();
		});
	}

	void LastFM::logout() {
		if(!_session) {
			return;
		}
		_session = std::nullopt;
		saveSession();
	}

	Optional<LastFMSession> LastFM::session() const {
		return _session;
	}


	Promise<LastFMScrobbleResponse> LastFM::scrobble(LastFMScrobbleRequest request) {
		return sendRequest(utils::HttpMethod::POST, "track.scrobble", request.toQueryItems()).map(nullptr, [](Json response) {
			return LastFMScrobbleResponse::fromJson(response);
		});
	}
}

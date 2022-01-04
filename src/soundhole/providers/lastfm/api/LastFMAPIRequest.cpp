//
//  LastFMRequest.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/6/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "LastFMAPIRequest.hpp"
#include "LastFMError.hpp"

namespace sh {
	utils::HttpRequest LastFMAPIRequest::toHttpRequest() const {
		auto url = apiRoot;
		if(apiMethod.empty()) {
			throw std::invalid_argument("apiMethod cannot be empty");
		}
		auto params = this->params;
		params["method"] = apiMethod;
		params["format"] = "json";
		if(session) {
			// add session key
			params["sk"] = session->key;
		}
		if(credentials) {
			// add api key
			params["api_key"] = credentials->key;
		}
		if(includeSignature) { // create signature
			String requestSig;
			auto ignoredParams = ArrayList<String>{ "api_sig", "format" };
			for(auto& pair : params) {
				if(ignoredParams.contains(pair.first)) {
					continue;
				}
				requestSig += pair.first;
				requestSig += pair.second;
			}
			if(credentials) {
				requestSig += credentials->secret;
			}
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
		utils::HttpHeaders headers;
		if(body.length() > 0) {
			headers.set("Content-Type", "application/x-www-form-urlencoded");
			headers.set("Content-Length", std::to_string(body.length()));
		}
		
		// build request
		return utils::HttpRequest{
			.url = Url(url),
			.method = httpMethod,
			.headers = headers,
			.data = body
		};
	}

	Promise<Json> LastFMAPIRequest::perform() const {
		// perform request
		return utils::performHttpRequest(toHttpRequest()).map(nullptr, [](utils::SharedHttpResponse response) -> Json {
			// map response
			std::string parseError;
			auto json = Json::parse(response->data, parseError);
			auto errorJson = json["error"];
			if(response->statusCode >= 300 || errorJson.number_value() != 0) {
				// handle error
				auto messageJson = json["message"];
				if(messageJson.is_null()
				   || (messageJson.is_string() && messageJson.string_value().empty())) {
					messageJson = Json((std::string)response->statusMessage);
				}
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
}

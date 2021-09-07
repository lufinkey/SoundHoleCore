//
//  OAuthSession.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/13/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "OAuthSession.hpp"
#include <soundhole/utils/HttpClient.hpp>

namespace sh {
	OAuthSession::OAuthSession(String accessToken, TimePoint expireTime, String tokenType, String refreshToken, ArrayList<String> scopes)
	: accessToken(accessToken), expireTime(expireTime), tokenType(tokenType), refreshToken(refreshToken), scopes(scopes) {
		//
	}
	
	const String& OAuthSession::getAccessToken() const {
		return accessToken;
	}
	
	const OAuthSession::TimePoint& OAuthSession::getExpireTime() const {
		return expireTime;
	}

	const String& OAuthSession::getTokenType() const {
		return tokenType;
	}
	
	const String& OAuthSession::getRefreshToken() const {
		return refreshToken;
	}
	
	const ArrayList<String>& OAuthSession::getScopes() const {
		return scopes;
	}
	
	bool OAuthSession::isValid() const {
		return (expireTime > std::chrono::system_clock::now());
	}
	
	bool OAuthSession::hasRefreshToken() const {
		return (refreshToken.length() > 0);
	}
	
	bool OAuthSession::hasScope(String scope) const {
		return scopes.contains(scope);
	}
	
	void OAuthSession::update(String accessToken, TimePoint expireTime) {
		this->accessToken = accessToken;
		this->expireTime = expireTime;
	}
	
	OAuthSession::TimePoint OAuthSession::getExpireTimeFromSeconds(int seconds) {
		return std::chrono::system_clock::now() + std::chrono::seconds(seconds);
	}

	

	Promise<Json> OAuthSession::performTokenRequest(String url, std::map<String,String> params, std::map<String,String> headers) {
		utils::HttpRequest request;
		request.url = Url(url);
		request.method = utils::HttpMethod::POST;
		request.headers = utils::HttpHeaders(headers);
		request.headers.set("Content-Type", "application/x-www-form-urlencoded");
		request.data = utils::makeQueryString(params);
		return utils::performHttpRequest(request)
		.except([=](std::exception& error) -> utils::SharedHttpResponse {
			throw OAuthError(OAuthError::Code::REQUEST_NOT_SENT, error.what(), {
				{ "url", url },
				{ "params", params }
			});
		})
		.map([=](utils::SharedHttpResponse response) -> Json {
			std::string parseError;
			auto json = Json::parse(response->data, parseError);
			auto error = json["error"];
			if(response->statusCode < 200 || response->statusCode >= 300 || error.is_string()) {
				if(error.is_string()) {
					OAuthError::Code errorCode = OAuthError::Code_fromOAuthError(error.string_value()).valueOr(OAuthError::Code::REQUEST_FAILED);
					String errorMessage = json["error_description"].string_value();
					if(errorMessage.empty()) {
						errorMessage = OAuthError::Code_getDescription(errorCode);
					}
					throw OAuthError(errorCode, errorMessage, {
						{ "statusCode", int(response->statusCode) },
						{ "oauthError", String(error.string_value()) },
						{ "url", url },
						{ "httpRequest", request },
						{ "httpResponse", response }
					});
				} else {
					throw OAuthError(OAuthError::Code::REQUEST_FAILED, response->statusMessage, {
						{ "httpResponse", response }
					});
				}
			}
			if(parseError.length() > 0 && response->data.length() > 0) {
				throw OAuthError(OAuthError::Code::BAD_DATA, parseError, {
					{ "httpResponse", response }
				});
			}
			return json;
		});
	}


	Promise<OAuthSession> OAuthSession::swapCodeForToken(String url, String code, std::map<String,String> params, std::map<String,String> headers) {
		params.insert_or_assign("code", code);
		return performTokenRequest(url, params, headers).map([](Json result) -> OAuthSession {
			auto resultShape = std::initializer_list<std::pair<std::string,Json::Type>>{
				{ "access_token", Json::Type::STRING },
				{ "expires_in", Json::Type::NUMBER },
				{ "token_type", Json::Type::STRING }
			};
			if(!result.is_object()) {
				throw OAuthError(OAuthError::Code::BAD_DATA, "Token swap result is not an object");
			}
			std::string shapeError;
			if(!result.has_shape(resultShape, shapeError)) {
				throw OAuthError(OAuthError::Code::BAD_DATA, "Incorrect shape for token swap result: "+shapeError, {
					{ "jsonResponse", result }
				});
			}
			String accessToken = result["access_token"].string_value();
			int expireSeconds = (int)result["expires_in"].number_value();
			String tokenType = result["token_type"].string_value();
			String refreshToken;
			Json refreshTokenVal = result["refresh_token"];
			if(refreshTokenVal.is_string()) {
				refreshToken = refreshTokenVal.string_value();
			}
			ArrayList<String> scopes;
			Json scopeVal = result["scope"];
			if(scopeVal.is_string()) {
				scopes = String(scopeVal.string_value()).split(' ');
			}
			return OAuthSession(accessToken, OAuthSession::getExpireTimeFromSeconds(expireSeconds), tokenType, refreshToken, scopes);
		});
	}


	Promise<OAuthSession> OAuthSession::refreshSession(String url, OAuthSession session, std::map<String,String> params, std::map<String,String> headers) {
		if(session.refreshToken.empty()) {
			return Promise<OAuthSession>::reject(
				OAuthError(OAuthError::Code::INVALID_REQUEST, "missing refresh token"));
		}
		params.insert_or_assign("refresh_token", session.refreshToken);
		params.insert_or_assign("grant_type", "refresh_token");
		return performTokenRequest(url, params, headers).map([=](Json result) -> OAuthSession {
			auto accessToken = result["access_token"];
			auto expireSeconds = result["expires_in"];
			if(!accessToken.is_string() || !expireSeconds.is_number()) {
				throw OAuthError(OAuthError::Code::BAD_DATA, "token refresh response does not match expected shape", {
					{ "jsonResponse", result }
				});
			}
			auto expireTime = OAuthSession::getExpireTimeFromSeconds((int)expireSeconds.number_value());
			// token type
			String tokenType = session.tokenType;
			auto tokenTypeVal = result["token_type"];
			if(!tokenTypeVal.is_null()) {
				tokenType = tokenTypeVal.string_value();
			}
			// scope
			ArrayList<String> scopes = session.scopes;
			auto scopesVal = result["scope"];
			if(!scopesVal.is_null()) {
				scopes = String(scopesVal.string_value()).split(' ');
			}
			// create session
			return OAuthSession(accessToken.string_value(), expireTime, tokenType, session.refreshToken, scopes);
		});
	}
}

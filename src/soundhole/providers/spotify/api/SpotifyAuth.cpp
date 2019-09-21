//
//  SpotifyAuth.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SpotifyAuth.hpp"
#include "SpotifyError.hpp"
#include <soundhole/utils/HttpClient.hpp>
#include <soundhole/utils/Utils.hpp>

namespace sh {
	String SpotifyAuth::Options::getWebAuthenticationURL(String state) const {
		std::map<String,String> query;
		if(clientId.length() > 0) {
			query.emplace("client_id", clientId);
		}
		if(tokenSwapURL.length() > 0) {
			query.emplace("response_type", "code");
		} else {
			query.emplace("response_type", "token");
		}
		if(redirectURL.length() > 0) {
			query.emplace("redirect_uri", redirectURL);
		}
		if(scopes.size() > 0) {
			query.emplace("scope", String::join(scopes, " "));
		}
		if(state.length() > 0) {
			query.emplace("state", state);
		}
		for(auto& pair : params) {
			auto it = query.find(pair.first);
			if(it != query.end()) {
				it->second = pair.second.toStdString<char>();
			} else {
				query.emplace(pair.first, pair.second);
			}
		}
		return "https://accounts.spotify.com/authorize?" + utils::makeQueryString(query);
	}
	
	
	
	
	SpotifyAuth::SpotifyAuth(Options options): options(options) {
		//
	}
	
	bool SpotifyAuth::Options::hasTokenSwapURL() const {
		return (tokenSwapURL.length() > 0);
	}
	
	bool SpotifyAuth::Options::hasTokenRefreshURL() const {
		return (tokenRefreshURL.length() > 0);
	}
	
	bool SpotifyAuth::isLoggedIn() const {
		return session.has_value();
	}
	
	bool SpotifyAuth::isSessionValid() const {
		return session.has_value() && session->isValid();
	}
	
	bool SpotifyAuth::hasStreamingScope() const {
		return session->hasScope("streaming");
	}
	
	bool SpotifyAuth::canRefreshSession() const {
		return session.has_value() && session->hasRefreshToken() && options.hasTokenRefreshURL();
	}
	
	
	
	
	void SpotifyAuth::startSession(SpotifySession newSession) {
		session = newSession;
		save();
	}
	
	void SpotifyAuth::clearSession() {
		session.reset();
		save();
	}
	
	
	
	
	Promise<bool> SpotifyAuth::renewSession(RenewOptions options) {
		throw std::runtime_error("not implemented");
		/*return Promise<bool>([&](auto resolve, auto reject) {
			if(!canRefreshSession()) {
				resolve(false);
				return;
			}
			bool shouldPerformRenewal = false;
			std::unique_lock<std::mutex> lock(renewalInfoMutex);
			if(!renewalInfo) {
				shouldPerformRenewal = true;
				renewalInfo = std::make_shared<RenewalInfo>();
			}
			if(options.retryUntilResponse) {
				renewalInfo->retryUntilResponseCallbacks.pushBack({ resolve, reject });
			} else {
				renewalInfo->callbacks.pushBack({ resolve, reject });
			}
			lock.unlock();
			if(!shouldPerformRenewal) {
				return;
			}
			
			// TODO perform actual session renewal
		});*/
	}
	
	Promise<bool> SpotifyAuth::renewSessionIfNeeded(RenewOptions options) {
		throw std::runtime_error("not implemented");
	}
	
	
	
	
	Promise<SpotifySession> SpotifyAuth::swapCodeForToken(String code, String url) {
		auto params = std::map<String,String>{
			{ "code", code }
		};
		return performTokenURLRequest(url, params).map<SpotifySession>([](Json result) -> SpotifySession {
			auto resultShape = std::initializer_list<std::pair<std::string,Json::Type>>{
				{ "access_token", Json::Type::STRING },
				{ "expires_in", Json::Type::NUMBER }
			};
			if(!result.is_object()) {
				throw std::runtime_error("Bad response for token swap: result is not an object");
			}
			std::string shapeError;
			if(!result.has_shape(resultShape, shapeError)) {
				throw std::runtime_error("Bad response for token swap: "+shapeError);
			}
			String accessToken = result["access_token"].string_value();
			int expireSeconds = (int)result["expires_in"].number_value();
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
			return SpotifySession(accessToken, SpotifySession::getExpireTimeFromSeconds(expireSeconds), refreshToken, scopes);
		});
	}
	
	Promise<Json> SpotifyAuth::performTokenURLRequest(String url, std::map<String,String> params) {
		utils::HttpRequest request;
		request.url = Url(url);
		request.method = utils::HttpMethod::POST;
		request.data = utils::makeQueryString(params);
		return utils::performHttpRequest(request).map<Json>([](std::shared_ptr<utils::HttpResponse> response) -> Json {
			std::string parseError;
			auto json = Json::parse(response->data.storage, parseError);
			if(parseError.length() > 0) {
				throw SpotifyError(SpotifyError::Code::BAD_RESPONSE, parseError, {
					{ "httpResponse", response }
				});
			}
			auto error = json["error"];
			if(error.is_string()) {
				auto error_description = json["error_description"];
				auto error_uri = json["error_uri"];
				throw SpotifyError(SpotifyError::Code::OAUTH_REQUEST_FAILED, error_description.string_value(), {
					{ "statusCode", int(response->statusCode) },
					{ "oauthError", String(error.string_value()) }
				});
			}
			return json;
		}).except([=](std::exception& error) -> Json {
			throw SpotifyError(SpotifyError::Code::REQUEST_NOT_SENT, error.what(), {
				{ "url", url },
				{ "params", params }
			});
		});
	}
}

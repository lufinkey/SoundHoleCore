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
	
	SpotifyAuth::~SpotifyAuth() {
		if(renewalInfo != nullptr) {
			renewalInfo->deleted = true;
		}
	}
	
	void SpotifyAuth::addEventListener(SpotifyAuthEventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		listeners.pushBack(listener);
	}
	
	void SpotifyAuth::removeEventListener(SpotifyAuthEventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		listeners.removeEqual(listener);
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
		return Promise<bool>([&](auto resolve, auto reject) {
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
			
			performSessionRenewal().then(resolve, reject);
		});
	}
	
	Promise<bool> SpotifyAuth::renewSessionIfNeeded(RenewOptions options) {
		if(!session || isSessionValid()) {
			// not logged in or session does not need renewal
			return Promise<bool>::resolve(false);
		} else if(!canRefreshSession()) {
			// no refresh token to renew session with, so the session has expired
			return Promise<bool>::reject(SpotifyError(SpotifyError::Code::SESSION_EXPIRED, "Spotify session has expired"));
		} else {
			return renewSession(options);
		}
	}
	
	Promise<bool> SpotifyAuth::performSessionRenewal() {
		std::unique_lock<std::mutex> lock(this->renewalInfoMutex);
		// ensure session can be refreshed
		if(!canRefreshSession()) {
			// session ended, so call all callbacks with false and return
			std::shared_ptr<RenewalInfo> renewalInfo;
			renewalInfo.swap(this->renewalInfo);
			lock.unlock();
			for(auto& callback : renewalInfo->callbacks) {
				callback.resolve(false);
			}
			for(auto& callback : renewalInfo->retryUntilResponseCallbacks) {
				callback.resolve(false);
			}
			return Promise<bool>::resolve(false);
		}
		// save renewalInfo pointer
		auto renewalInfo = this->renewalInfo;
		lock.unlock();
		// perform session renewal request
		return performTokenURLRequest(this->options.tokenRefreshURL, {
			{ "refresh_token", this->session->getRefreshToken() }
		}).map<bool>([=](Json result) -> bool {
			// ensure session still exists
			if(renewalInfo->deleted || !canRefreshSession()) {
				// session ended, so call all callbacks with false and return
				if(!renewalInfo->deleted) {
					std::unique_lock<std::mutex> lock(this->renewalInfoMutex);
					this->renewalInfo = nullptr;
					lock.unlock();
				}
				for(auto& callback : renewalInfo->callbacks) {
					callback.resolve(false);
				}
				for(auto& callback : renewalInfo->retryUntilResponseCallbacks) {
					callback.resolve(false);
				}
				return false;
			}
			
			// get session renewal data
			auto accessToken = result["access_token"];
			auto expireSeconds = result["expires_in"];
			if(!accessToken.is_string() || !expireSeconds.is_number()) {
				throw SpotifyError(SpotifyError::Code::BAD_RESPONSE, "token refresh response does not match expected shape", {
					{ "jsonResponse", result }
				});
			}
			this->session->update(accessToken.string_value(), SpotifySession::getExpireTimeFromSeconds((int)expireSeconds.number_value()));
			this->save();
			
			// get renewal callbacks
			std::unique_lock<std::mutex> lock(this->renewalInfoMutex);
			std::shared_ptr<RenewalInfo> renewalInfo;
			renewalInfo.swap(this->renewalInfo);
			lock.unlock();
			
			// call renewal callbacks
			for(auto& callback : renewalInfo->callbacks) {
				callback.resolve(true);
			}
			for(auto& callback : renewalInfo->retryUntilResponseCallbacks) {
				callback.resolve(true);
			}
			
			// call listeners
			std::unique_lock<std::mutex> listenersLock(listenersMutex);
			LinkedList<SpotifyAuthEventListener*> listeners = this->listeners;
			listenersLock.unlock();
			for(auto listener : listeners) {
				listener->onSpotifyAuthSessionRenewed(this);
			}
			
			// renewal is successful
			return true;
		}).except([=](std::exception_ptr errorPtr) -> Promise<bool> {
			// get renewal callbacks
			bool shouldRetry = false;
			std::mutex* mutexPtr = nullptr;
			if(!renewalInfo->deleted) {
				mutexPtr = &this->renewalInfoMutex;
			}
			LinkedList<RenewCallback> callbacks;
			std::mutex defaultMutex;
			std::unique_lock<std::mutex> lock((mutexPtr != nullptr) ? *mutexPtr : defaultMutex);
			callbacks.swap(renewalInfo->callbacks);
			try {
				std::rethrow_exception(errorPtr);
			} catch(SpotifyError& error) {
				// if the request wasn't sent, we should try again if needed, otherwise give up and forward the error
				if(error.getCode() == SpotifyError::Code::REQUEST_NOT_SENT) {
					if(renewalInfo->retryUntilResponseCallbacks.size() > 0) {
						shouldRetry = true;
					} else {
						if(!renewalInfo->deleted) {
							this->renewalInfo = nullptr;
						}
					}
				} else {
					callbacks.splice(callbacks.end(), renewalInfo->retryUntilResponseCallbacks);
					if(!renewalInfo->deleted) {
						this->renewalInfo = nullptr;
					}
				}
			} catch(...) {
				// just forward the other errors
			}
			lock.unlock();
			
			// call renewal callbacks
			for(auto& callback : callbacks) {
				callback.reject(errorPtr);
			}
			
			// retry if needed, otherwise fail
			if(shouldRetry) {
				if(renewalInfo->deleted) {
					return Promise<bool>::resolve(false);
				}
				// wait 2 seconds before retrying
				return Promise<bool>([&](auto resolve, auto reject) {
					getDefaultPromiseQueue()->asyncAfter(std::chrono::steady_clock::now() + std::chrono::seconds(2), [=]() {
						if(!renewalInfo->deleted) {
							this->performSessionRenewal().then(resolve, reject);
						} else {
							Promise<bool>::resolve(false).then(resolve, reject);
						}
					});
				});
			}
			return Promise<bool>::reject(errorPtr);
		});
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
			if((response->statusCode < 200 || response->statusCode >= 300) && response->data.size() == 0) {
				throw SpotifyError(SpotifyError::Code::OAUTH_REQUEST_FAILED, response->statusMessage, {
					{ "httpResponse", response }
				});
			}
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

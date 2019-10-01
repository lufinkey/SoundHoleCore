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
	
	
	
	void SpotifyAuth::load() {
		if(options.sessionPersistKey.empty()) {
			return;
		}
		bool wasLoggedIn = isLoggedIn();
		session = SpotifySession::load(options.sessionPersistKey);
		if(!wasLoggedIn && session) {
			std::unique_lock<std::mutex> lock(listenersMutex);
			LinkedList<SpotifyAuthEventListener*> listeners = this->listeners;
			lock.unlock();
			for(auto listener : listeners) {
				listener->onSpotifyAuthSessionResume(this);
			}
			if(session && !session->isValid()) {
				std::unique_lock<std::mutex> lock(listenersMutex);
				LinkedList<SpotifyAuthEventListener*> listeners = this->listeners;
				lock.unlock();
				for(auto listener : listeners) {
					listener->onSpotifyAuthSessionExpire(this);
				}
			}
		}
		renewSessionIfNeeded({.retryUntilResponse = true});
	}
	
	void SpotifyAuth::save() {
		if(options.sessionPersistKey.empty()) {
			return;
		}
		SpotifySession::save(options.sessionPersistKey, session);
	}
	
	
	
	void SpotifyAuth::addEventListener(SpotifyAuthEventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		listeners.pushBack(listener);
	}
	
	void SpotifyAuth::removeEventListener(SpotifyAuthEventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		listeners.removeFirstEqual(listener);
	}
	
	bool SpotifyAuth::Options::hasTokenSwapURL() const {
		return (tokenSwapURL.length() > 0);
	}
	
	bool SpotifyAuth::Options::hasTokenRefreshURL() const {
		return (tokenRefreshURL.length() > 0);
	}
	
	const SpotifyAuth::Options& SpotifyAuth::getOptions() const {
		return this->options;
	}
	
	bool SpotifyAuth::isLoggedIn() const {
		return session.has_value();
	}
	
	bool SpotifyAuth::isSessionValid() const {
		return session.has_value() && session->isValid();
	}
	
	bool SpotifyAuth::hasStreamingScope() const {
		if(!session) {
			return false;
		}
		return session->hasScope("streaming");
	}
	
	bool SpotifyAuth::canRefreshSession() const {
		return session.has_value() && session->hasRefreshToken() && options.hasTokenRefreshURL();
	}
	
	
	
	
	void SpotifyAuth::startSession(SpotifySession newSession) {
		std::unique_lock<std::recursive_mutex> lock(sessionMutex);
		session = newSession;
		save();
		if(session) {
			startRenewalTimer();
		}
	}
	
	void SpotifyAuth::updateSession(String accessToken, SpotifySession::TimePoint expireTime) {
		std::unique_lock<std::recursive_mutex> lock(sessionMutex);
		if(!session) {
			return;
		}
		session->update(accessToken, expireTime);
		save();
		rescheduleRenewalTimer();
	}
	
	void SpotifyAuth::clearSession() {
		std::unique_lock<std::recursive_mutex> lock(sessionMutex);
		stopRenewalTimer();
		session.reset();
		save();
	}
	
	const Optional<SpotifySession>& SpotifyAuth::getSession() const {
		return session;
	}
	
	
	
	void SpotifyAuth::loginWithSession(SpotifySession newSession) {
		bool wasLoggedIn = isLoggedIn();
		startSession(newSession);
		if(!wasLoggedIn && session) {
			std::unique_lock<std::mutex> lock(listenersMutex);
			LinkedList<SpotifyAuthEventListener*> listeners = this->listeners;
			lock.unlock();
			for(auto listener : listeners) {
				listener->onSpotifyAuthSessionStart(this);
			}
			if(session && !session->isValid()) {
				std::unique_lock<std::mutex> lock(listenersMutex);
				LinkedList<SpotifyAuthEventListener*> listeners = this->listeners;
				lock.unlock();
				for(auto listener : listeners) {
					listener->onSpotifyAuthSessionExpire(this);
				}
			}
		}
		renewSessionIfNeeded({.retryUntilResponse = true});
	}
	
	void SpotifyAuth::logout() {
		bool wasLogged = isLoggedIn();
		clearSession();
		if(wasLogged) {
			std::unique_lock<std::mutex> lock(listenersMutex);
			LinkedList<SpotifyAuthEventListener*> listeners = this->listeners;
			lock.unlock();
			for(auto listener : listeners) {
				listener->onSpotifyAuthSessionEnd(this);
			}
		}
	}
	
	Promise<bool> SpotifyAuth::renewSession(RenewOptions options) {
		return Promise<bool>([&](auto resolve, auto reject) {
			if(!canRefreshSession()) {
				resolve(false);
				return;
			}
			
			bool shouldPerformRenewal = false;
			std::unique_lock<std::recursive_mutex> lock(sessionMutex);
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
		std::unique_lock<std::recursive_mutex> lock(sessionMutex);
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
			auto cancelRenewal = [&]() {
				if(!renewalInfo->deleted) {
					std::unique_lock<std::recursive_mutex> lock(this->sessionMutex);
					this->renewalInfo = nullptr;
					lock.unlock();
				}
				for(auto& callback : renewalInfo->callbacks) {
					callback.resolve(false);
				}
				for(auto& callback : renewalInfo->retryUntilResponseCallbacks) {
					callback.resolve(false);
				}
			};
			if(renewalInfo->deleted || !canRefreshSession()) {
				// session ended, so call all callbacks with false and return
				cancelRenewal();
				return false;
			}
			std::unique_lock<std::recursive_mutex> lock(this->sessionMutex);
			if(!canRefreshSession()) {
				// session ended, so call all callbacks with false and return
				cancelRenewal();
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
			this->updateSession(accessToken.string_value(), SpotifySession::getExpireTimeFromSeconds((int)expireSeconds.number_value()));
			
			// get renewal callbacks
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
				listener->onSpotifyAuthSessionRenew(this);
			}
			
			// renewal is successful
			return true;
		}).except([=](std::exception_ptr errorPtr) -> Promise<bool> {
			// get renewal callbacks
			bool shouldRetry = false;
			std::recursive_mutex* mutexPtr = nullptr;
			if(!renewalInfo->deleted) {
				mutexPtr = &this->sessionMutex;
			}
			LinkedList<WaitCallback> callbacks;
			std::recursive_mutex defaultMutex;
			std::unique_lock<std::recursive_mutex> lock((mutexPtr != nullptr) ? *mutexPtr : defaultMutex);
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
		request.headers["Content-Type"] = "application/x-www-form-urlencoded";
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
	
	
	
	void SpotifyAuth::startRenewalTimer() {
		if(renewalTimer && renewalTimer->isValid()) {
			// auth renewal timer has already been started, don't bother starting again
			return;
		}
		rescheduleRenewalTimer();
	}
	
	void SpotifyAuth::rescheduleRenewalTimer() {
		if(!canRefreshSession()) {
			// we can't perform token refresh, so don't bother scheduling the timer
			return;
		}
		auto now = std::chrono::system_clock::now();
		auto expireTime = session->getExpireTime();
		auto timeDiff = expireTime - now;
		auto renewalTimeDiff = (expireTime - options.tokenRefreshEarliness) - now;
		if(timeDiff <= std::chrono::seconds(30) || timeDiff <= (options.tokenRefreshEarliness + std::chrono::seconds(30)) || renewalTimeDiff <= std::chrono::seconds(0)) {
			onRenewalTimerFire();
		}
		else {
			if(renewalTimer) {
				renewalTimer->cancel();
			}
			renewalTimer = Timer::withTimeout(renewalTimeDiff, [=](auto timer) {
				onRenewalTimerFire();
			});
		}
	}
	
	void SpotifyAuth::stopRenewalTimer() {
		if(renewalTimer) {
			renewalTimer->cancel();
			renewalTimer = nullptr;
		}
	}
	
	void SpotifyAuth::onRenewalTimerFire() {
		renewSession({.retryUntilResponse=true});
	}
}

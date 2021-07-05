//
//  YoutubeAuth.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/12/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "YoutubeAuth.hpp"
#include "YoutubeError.hpp"
#include <soundhole/utils/HttpClient.hpp>
#include <soundhole/utils/Utils.hpp>

namespace sh {
	YoutubeAuth::AuthenticateOptions YoutubeAuth::Options::getAuthenticateOptions() const {
		return {
			.clientId=clientId,
			.clientSecret=clientSecret,
			.redirectURL=redirectURL,
			.scopes=scopes,
			.tokenSwapURL=tokenSwapURL
		};
	}

	String YoutubeAuth::AuthenticateOptions::getWebAuthenticationURL(String codeChallenge) const {
		auto query = std::map<String,String>{
			{ "client_id", clientId },
			{ "redirect_uri", redirectURL },
			{ "response_type", "code" },
			{ "scope", String::join(scopes, " ") },
			{ "access_type", "offline" }
		};
		if(!codeChallenge.empty()) {
			query.insert_or_assign("code_challenge", codeChallenge);
			query.insert_or_assign("code_challenge_method", "plain");
		}
		return "https://accounts.google.com/o/oauth2/v2/auth?" + utils::makeQueryString(query);
	}
	
	
	
	YoutubeAuth::YoutubeAuth(Options options)
	: options(options), sessionMgr(this) {
		//
	}
	
	
	
	void YoutubeAuth::load() {
		sessionMgr.load();
	}
	void YoutubeAuth::save() {
		sessionMgr.save();
	}
	
	
	
	void YoutubeAuth::addEventListener(YoutubeAuthEventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		listeners.pushBack(listener);
	}
	void YoutubeAuth::removeEventListener(YoutubeAuthEventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		listeners.removeLastEqual(listener);
	}



	const YoutubeAuth::Options& YoutubeAuth::getOptions() const {
		return this->options;
	}
	bool YoutubeAuth::isLoggedIn() const {
		return sessionMgr.getSession().hasValue();
	}
	bool YoutubeAuth::isSessionValid() const {
		return sessionMgr.isSessionValid();
	}
	const Optional<YoutubeSession>& YoutubeAuth::getSession() const {
		return sessionMgr.getSession();
	}



	Promise<bool> YoutubeAuth::login() {
		return authenticate(options.getAuthenticateOptions()).map([=](Optional<YoutubeSession> session) -> bool {
			if(session) {
				loginWithSession(session.value());
			}
			return session.hasValue();
		});
	}
	void YoutubeAuth::loginWithSession(YoutubeSession newSession) {
		sessionMgr.startSession(newSession);
	}
	void YoutubeAuth::logout() {
		sessionMgr.endSession();
	}

	Promise<bool> YoutubeAuth::renewSession(RenewOptions options) {
		return sessionMgr.renewSession(options);
	}
	Promise<bool> YoutubeAuth::renewSessionIfNeeded(RenewOptions options) {
		return sessionMgr.renewSessionIfNeeded(options);
	}



	Promise<YoutubeSession> YoutubeAuth::swapCodeForToken(String code, String codeVerifier, AuthenticateOptions options) {
		auto params = std::map<String,String>{
			{ "grant_type", "authorization_code" }
		};
		if(!options.clientId.empty()) {
			params["client_id"] = options.clientId;
		}
		if(!codeVerifier.empty()) {
			params["code_verifier"] = codeVerifier;
		}
		if(!options.clientSecret.empty()) {
			params["client_secret"] = options.clientSecret;
		}
		if(!options.redirectURL.empty()) {
			params["redirect_uri"] = options.redirectURL;
		}
		if(options.tokenSwapURL.empty()) {
			options.tokenSwapURL = "https://oauth2.googleapis.com/token";
		}
		return YoutubeSession::swapCodeForToken(options.tokenSwapURL, code, params, {});
	}



	Promise<Optional<YoutubeSession>> YoutubeAuth::handleOAuthRedirect(std::map<String,String> params, AuthenticateOptions options, String codeVerifier) {
		return Promise<void>::resolve().then([=]() -> Promise<Optional<YoutubeSession>> {
			auto error = utils::getMapValueOrDefault<String,String>(params, "error", String());
			auto code = utils::getMapValueOrDefault<String,String>(params, "code", String());
			if(!error.empty()) {
				if(error == "access_denied") {
					// denied from webview
					return Promise<Optional<YoutubeSession>>::reject(std::runtime_error("Access Denied"));
				}
				else {
					// error
					return Promise<Optional<YoutubeSession>>::reject(
						OAuthError(OAuthError::Code::REQUEST_FAILED, String(error)));
				}
			}
			else if(!code.empty()) {
				// authentication code
				return swapCodeForToken(code, codeVerifier, options).map([=](YoutubeSession session) -> Optional<YoutubeSession> {
					if(session.getScopes().empty() && !options.scopes.empty()) {
						return YoutubeSession(
							session.getAccessToken(),
							session.getExpireTime(),
							session.getTokenType(),
							session.getRefreshToken(),
							options.scopes);
					}
					return session;
				});
			}
			else {
				// missing parameters
				return Promise<Optional<YoutubeSession>>::reject(
					OAuthError(OAuthError::Code::BAD_DATA, "Missing expected parameters in redirect URL"));
			}
		});
	}



	#pragma mark OAuthSessionManager::Delegate

	String YoutubeAuth::getOAuthSessionPersistKey(const OAuthSessionManager* mgr) const {
		return options.sessionPersistKey;
	}

	std::map<String,String> YoutubeAuth::getOAuthTokenRefreshParams(const OAuthSessionManager* mgr) const {
		auto params = std::map<String,String>{
			{ "client_id", options.clientId }
		};
		if(!options.clientSecret.empty()) {
			params["client_secret"] = options.clientSecret;
		}
		return params;
	}

	std::map<String,String> YoutubeAuth::getOAuthTokenRefreshHeaders(const OAuthSessionManager* mgr) const {
		return {};
	}

	std::chrono::seconds YoutubeAuth::getOAuthTokenRefreshEarliness(const OAuthSessionManager* mgr) const {
		return options.tokenRefreshEarliness;
	}

	String YoutubeAuth::getOAuthTokenRefreshURL(const OAuthSessionManager* mgr) const {
		if(options.tokenRefreshURL.empty()) {
			return "https://oauth2.googleapis.com/token";
		}
		return options.tokenRefreshURL;
	}



	void YoutubeAuth::onOAuthSessionStart(OAuthSessionManager* mgr) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		auto listeners = this->listeners;
		lock.unlock();
		for(auto listener : listeners) {
			listener->onYoutubeAuthSessionStart(this);
		}
	}
	void YoutubeAuth::onOAuthSessionResume(OAuthSessionManager* mgr) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		auto listeners = this->listeners;
		lock.unlock();
		for(auto listener : listeners) {
			listener->onYoutubeAuthSessionResume(this);
		}
	}
	void YoutubeAuth::onOAuthSessionRenew(OAuthSessionManager* mgr) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		auto listeners = this->listeners;
		lock.unlock();
		for(auto listener : listeners) {
			listener->onYoutubeAuthSessionRenew(this);
		}
	}
	void YoutubeAuth::onOAuthSessionExpire(OAuthSessionManager* mgr) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		auto listeners = this->listeners;
		lock.unlock();
		for(auto listener : listeners) {
			listener->onYoutubeAuthSessionExpire(this);
		}
	}
	void YoutubeAuth::onOAuthSessionEnd(OAuthSessionManager* mgr) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		auto listeners = this->listeners;
		lock.unlock();
		for(auto listener : listeners) {
			listener->onYoutubeAuthSessionEnd(this);
		}
	}
}

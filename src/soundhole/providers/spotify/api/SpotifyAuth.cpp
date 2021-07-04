//
//  SpotifyAuth.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SpotifyAuth.hpp"
#include "SpotifyError.hpp"
#include <soundhole/utils/Base64.hpp>
#include <soundhole/utils/HttpClient.hpp>
#include <soundhole/utils/Utils.hpp>

namespace sh {
	SpotifyAuth::AuthenticateOptions SpotifyAuth::Options::getAuthenticateOptions() const {
		return {
			.clientId=clientId,
			.clientSecret=clientSecret,
			.redirectURL=redirectURL,
			.scopes=scopes,
			.showDialog=showLoginDialog,
			.tokenSwapURL=tokenSwapURL,
			.android=android
		};
	}

	String SpotifyAuth::AuthenticateOptions::getWebAuthenticationURL(String xssState) const {
		auto params = std::map<String,String>{
			{ "client_id", clientId },
			{ "redirect_uri", redirectURL },
			{ "response_type", "code" },
			{ "scope", String::join(scopes, " ") }
		};
		if(showDialog) {
			params.insert_or_assign("show_dialog", showDialog.value() ? "true" : "false");
		}
		if(!xssState.empty()) {
			params.insert_or_assign("state", xssState);
		}
		return "https://accounts.spotify.com/authorize?" + utils::makeQueryString(params);
	}
	
	
	
	
	SpotifyAuth::SpotifyAuth(Options options)
	: options(options), sessionMgr(this) {
		//
	}



	void SpotifyAuth::load() {
		sessionMgr.load();
	}
	void SpotifyAuth::save() {
		sessionMgr.save();
	}



	void SpotifyAuth::addEventListener(SpotifyAuthEventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		listeners.pushBack(listener);
	}
	void SpotifyAuth::removeEventListener(SpotifyAuthEventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		listeners.removeLastEqual(listener);
	}



	const SpotifyAuth::Options& SpotifyAuth::getOptions() const {
		return this->options;
	}
	bool SpotifyAuth::isLoggedIn() const {
		return sessionMgr.getSession().hasValue();
	}
	bool SpotifyAuth::isSessionValid() const {
		return sessionMgr.isSessionValid();
	}
	bool SpotifyAuth::hasStreamingScope() const {
		auto session = sessionMgr.getSession();
		if(!session) {
			return false;
		}
		return session->hasScope("streaming");
	}
	bool SpotifyAuth::canRefreshSession() const {
		return sessionMgr.canRefreshSession();
	}
	const Optional<SpotifySession>& SpotifyAuth::getSession() const {
		return sessionMgr.getSession();
	}



	Promise<Optional<SpotifySession>> SpotifyAuth::authenticate(LoginOptions options) {
		auto authOptions = this->options.getAuthenticateOptions();
		if(options.showDialog.hasValue()) {
			authOptions.showDialog = options.showDialog;
		}
		return SpotifyAuth::authenticate(authOptions);
	}
	Promise<bool> SpotifyAuth::login(LoginOptions options) {
		return this->authenticate(options).map([=](Optional<SpotifySession> session) -> bool {
			if(session) {
				loginWithSession(session.value());
			}
			return session.hasValue();
		});
	}
	void SpotifyAuth::loginWithSession(SpotifySession newSession) {
		sessionMgr.startSession(newSession);
	}
	void SpotifyAuth::logout() {
		sessionMgr.endSession();
	}
	
	Promise<bool> SpotifyAuth::renewSession(RenewOptions options) {
		return sessionMgr.renewSession(options);
	}
	Promise<bool> SpotifyAuth::renewSessionIfNeeded(RenewOptions options) {
		return sessionMgr.renewSessionIfNeeded(options);
	}



	Promise<SpotifySession> SpotifyAuth::swapCodeForToken(String code, AuthenticateOptions options) {
		auto params = std::map<String,String>{
			{ "grant_type", "authorization_code" }
		};
		if(!options.clientId.empty()) {
			params["client_id"] = options.clientId;
		}
		if(!options.redirectURL.empty()) {
			params["redirect_uri"] = options.redirectURL;
		}
		if(options.tokenSwapURL.empty()) {
			options.tokenSwapURL = "https://accounts.spotify.com/api/token";
		}
		auto headers = std::map<String,String>();
		if(!options.clientId.empty() && !options.clientSecret.empty()) {
			headers["Authorization"] = buildOAuthAuthorizationHeader(options.clientId, options.clientSecret);
		}
		return SpotifySession::swapCodeForToken(options.tokenSwapURL, code, params, headers);
	}


	String SpotifyAuth::buildOAuthAuthorizationHeader(String clientId, String clientSecret) {
		return "Basic "+utils::base64_encode(clientId+':'+clientSecret);
	}

	Promise<Optional<SpotifySession>> SpotifyAuth::handleOAuthRedirect(std::map<String,String> params, AuthenticateOptions options, String xssState) {
		return Promise<void>::resolve().then([=]() -> Promise<Optional<SpotifySession>> {
			auto state = utils::getMapValueOrDefault<String,String>(params, "state", String());
			auto error = utils::getMapValueOrDefault<String,String>(params, "error", String());
			auto accessToken = utils::getMapValueOrDefault<String,String>(params, "access_token", String());
			auto code = utils::getMapValueOrDefault<String,String>(params, "code", String());
			if(!error.empty()) {
				if(error == "access_denied") {
					// cancelled from webview
					return Promise<Optional<SpotifySession>>::resolve(std::nullopt);
				}
				else {
					// error
					return Promise<Optional<SpotifySession>>::reject(
						OAuthError(OAuthError::Code::REQUEST_FAILED, String(error)));
				}
			}
			else if(!xssState.empty() && xssState != state) {
				// state mismatch
				return Promise<Optional<SpotifySession>>::reject(
					OAuthError(OAuthError::Code::VERIFIER_MISMATCH, "State mismatch"));
			}
			else if(!accessToken.empty()) {
				// access token
				String expiresIn = params.at("expires_in");
				long expireSeconds = expiresIn.toArithmeticValue<long>();
				if(expireSeconds == 0) {
					return Promise<Optional<SpotifySession>>::reject(
						SpotifyError(SpotifyError::Code::BAD_DATA, "Access token expire time was 0"));
				}
				auto tokenType = params.at("token_type");
				auto scope = utils::getMapValueOrDefault<String,String>(params, "scope", String());
				ArrayList<String> scopes;
				if(!scope.empty()) {
					scopes = scope.split(' ');
				} else {
					scopes = options.scopes;
				}
				auto refreshToken = utils::getMapValueOrDefault<String,String>(params, "refreshToken", String());
				return Promise<Optional<SpotifySession>>::resolve(
					SpotifySession(
						String(accessToken),
						SpotifySession::getExpireTimeFromSeconds((int)expireSeconds),
						String(tokenType),
						refreshToken,
						scopes));
			}
			else if(!code.empty()) {
				// authentication code
				return swapCodeForToken(code, options).map([=](SpotifySession session) -> Optional<SpotifySession> {
					if(session.getScopes().empty() && !options.scopes.empty()) {
						return SpotifySession(
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
				return Promise<Optional<SpotifySession>>::reject(
					SpotifyError(SpotifyError::Code::BAD_DATA, "Missing expected parameters in redirect URL"));
			}
		});
	}



	#pragma mark OAuthSessionManager::Delegate

	String SpotifyAuth::getOAuthSessionPersistKey(const OAuthSessionManager* mgr) const {
		return options.sessionPersistKey;
	}

	std::map<String,String> SpotifyAuth::getOAuthTokenRefreshParams(const OAuthSessionManager* mgr) const {
		auto params = std::map<String,String>{
			{ "client_id", options.clientId }
		};
		if(!options.clientSecret.empty()) {
			params.insert_or_assign("client_secret", options.clientSecret);
		}
		return params;
	}

	std::map<String,String> SpotifyAuth::getOAuthTokenRefreshHeaders(const OAuthSessionManager* mgr) const {
		if(options.clientId.empty() || options.clientSecret.empty()) {
			return {};
		}
		return {
			{ "Authorization", buildOAuthAuthorizationHeader(options.clientId, options.clientSecret) }
		};
	}

	std::chrono::seconds SpotifyAuth::getOAuthTokenRefreshEarliness(const OAuthSessionManager* mgr) const {
		return options.tokenRefreshEarliness;
	}

	String SpotifyAuth::getOAuthTokenRefreshURL(const OAuthSessionManager* mgr) const {
		if(options.tokenRefreshURL.empty()) {
			return "https://accounts.spotify.com/api/token";
		}
		return options.tokenRefreshURL;
	}



	void SpotifyAuth::onOAuthSessionStart(OAuthSessionManager* mgr) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		auto listeners = this->listeners;
		lock.unlock();
		for(auto listener : listeners) {
			listener->onSpotifyAuthSessionStart(this);
		}
	}
	void SpotifyAuth::onOAuthSessionResume(OAuthSessionManager* mgr) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		auto listeners = this->listeners;
		lock.unlock();
		for(auto listener : listeners) {
			listener->onSpotifyAuthSessionResume(this);
		}
	}
	void SpotifyAuth::onOAuthSessionRenew(OAuthSessionManager* mgr) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		auto listeners = this->listeners;
		lock.unlock();
		for(auto listener : listeners) {
			listener->onSpotifyAuthSessionRenew(this);
		}
	}
	void SpotifyAuth::onOAuthSessionExpire(OAuthSessionManager* mgr) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		auto listeners = this->listeners;
		lock.unlock();
		for(auto listener : listeners) {
			listener->onSpotifyAuthSessionExpire(this);
		}
	}
	void SpotifyAuth::onOAuthSessionEnd(OAuthSessionManager* mgr) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		auto listeners = this->listeners;
		lock.unlock();
		for(auto listener : listeners) {
			listener->onSpotifyAuthSessionEnd(this);
		}
	}
}

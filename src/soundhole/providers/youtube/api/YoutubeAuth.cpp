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

namespace sh {
	String YoutubeAuth::Options::getWebAuthenticationURL(String codeChallenge) const {
		std::map<String,String> query = {
			{ "client_id", clientId },
			{ "redirect_uri", redirectURL },
			{ "response_type", "code" },
			{ "scopes", String::join(scopes, " ") },
			{ "code_challenge", codeChallenge },
			{ "code_challenge_method", "plain" }
		};
		for(auto& pair : params) {
			auto it = query.find(pair.first);
			if(it != query.end()) {
				it->second = pair.second.toStdString<char>();
			} else {
				query.emplace(pair.first, pair.second);
			}
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
		return authenticate({
			.clientId = options.clientId,
			.clientSecret = options.clientSecret,
			.redirectURL = options.redirectURL,
			.scopes = options.scopes
		}).map<bool>([=](Optional<YoutubeSession> session) -> bool {
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



	Promise<YoutubeSession> YoutubeAuth::swapCodeForToken(String code, TokenSwapOptions options) {
		auto params = std::map<String,String>{
			{ "client_id", options.clientId },
			{ "grant_type", "authorization_code" },
			{ "redirect_uri", options.redirectURL }
		};
		if(!options.codeVerifier.empty()) {
			params.insert_or_assign("code_verifier", options.codeVerifier);
		}
		if(!options.clientSecret.empty()) {
			params.insert_or_assign("client_secret", options.clientSecret);
		}
		return YoutubeSession::swapCodeForToken("https://oauth2.googleapis.com/token", code, params);
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
			params.insert_or_assign("client_secret", options.clientSecret);
		}
		return params;
	}

	std::chrono::seconds YoutubeAuth::getOAuthTokenRefreshEarliness(const OAuthSessionManager* mgr) const {
		return options.tokenRefreshEarliness;
	}

	String YoutubeAuth::getOAuthTokenRefreshURL(const OAuthSessionManager* mgr) const {
		return "https://oauth2.googleapis.com/token";
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

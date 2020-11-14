//
//  YoutubeAuth.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/12/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/utils/OAuthSessionManager.hpp>
#include "YoutubeAuthEventListener.hpp"
#include <chrono>

namespace sh {
	using YoutubeSession = OAuthSession;

	class YoutubeAuth: protected OAuthSessionManager::Delegate {
	public:
		struct AuthenticateOptions {
			String clientId;
			String clientSecret;
			String redirectURL;
			ArrayList<String> scopes;
			String tokenSwapURL;
			
			String getWebAuthenticationURL(String codeChallenge) const;
		};
		
		struct Options {
			String clientId;
			String clientSecret;
			String redirectURL;
			ArrayList<String> scopes;
			String sessionPersistKey;
			std::chrono::seconds tokenRefreshEarliness = std::chrono::seconds(300);
			
			String tokenSwapURL;
			String tokenRefreshURL;
			
			YoutubeAuth::AuthenticateOptions getAuthenticateOptions() const;
		};
		
		YoutubeAuth(Options options);
		
		void load();
		void save();
		
		void addEventListener(YoutubeAuthEventListener* listener);
		void removeEventListener(YoutubeAuthEventListener* listener);
		
		const Options& getOptions() const;
		bool isLoggedIn() const;
		bool isSessionValid() const;
		const Optional<YoutubeSession>& getSession() const;
		
		static Promise<Optional<YoutubeSession>> authenticate(AuthenticateOptions options);
		Promise<bool> login();
		void loginWithSession(YoutubeSession session);
		void logout();
		
		using RenewOptions = OAuthSessionManager::RenewOptions;
		Promise<bool> renewSession(RenewOptions options = RenewOptions{.retryUntilResponse=false});
		Promise<bool> renewSessionIfNeeded(RenewOptions options = RenewOptions{.retryUntilResponse=false});
		
		static Promise<YoutubeSession> swapCodeForToken(String code, String codeVerifier, AuthenticateOptions options);
		
	protected:
		virtual String getOAuthSessionPersistKey(const OAuthSessionManager* mgr) const override;
		virtual std::map<String,String> getOAuthTokenRefreshParams(const OAuthSessionManager* mgr) const override;
		virtual std::map<String,String> getOAuthTokenRefreshHeaders(const OAuthSessionManager* mgr) const override;
		virtual std::chrono::seconds getOAuthTokenRefreshEarliness(const OAuthSessionManager* mgr) const override;
		virtual String getOAuthTokenRefreshURL(const OAuthSessionManager* mgr) const override;
		
		virtual void onOAuthSessionStart(OAuthSessionManager* mgr) override;
		virtual void onOAuthSessionResume(OAuthSessionManager* mgr) override;
		virtual void onOAuthSessionRenew(OAuthSessionManager* mgr) override;
		virtual void onOAuthSessionExpire(OAuthSessionManager* mgr) override;
		virtual void onOAuthSessionEnd(OAuthSessionManager* mgr) override;
		
	private:
		static Promise<Optional<YoutubeSession>> handleOAuthRedirect(std::map<String,String> params, AuthenticateOptions options, String codeVerifier);
		
		Options options;
		OAuthSessionManager sessionMgr;
		
		LinkedList<YoutubeAuthEventListener*> listeners;
		std::mutex listenersMutex;
	};
}

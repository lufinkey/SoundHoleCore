//
//  SpotifyAuth.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/utils/OAuthSessionManager.hpp>
#include "SpotifyAuthEventListener.hpp"
#include <chrono>

namespace sh {
	using SpotifySession = OAuthSession;

	class SpotifyAuth: protected OAuthSessionManager::Delegate {
	public:
		struct AuthenticateOptions {
			String clientId;
			String clientSecret;
			String redirectURL;
			ArrayList<String> scopes;
			Optional<bool> showDialog;
			
			String tokenSwapURL;
			
			struct Android {
				String loginLoadingText;
			} android;
			
			String getWebAuthenticationURL(String xssState) const;
		};
		
		struct Options {
			String clientId;
			String clientSecret;
			String redirectURL;
			ArrayList<String> scopes;
			String sessionPersistKey;
			std::chrono::seconds tokenRefreshEarliness = std::chrono::seconds(300);
			Optional<bool> showLoginDialog;
			
			String tokenSwapURL;
			String tokenRefreshURL;

			using Android = AuthenticateOptions::Android;
			Android android;
			
			SpotifyAuth::AuthenticateOptions getAuthenticateOptions() const;
		};
		
		SpotifyAuth(Options options);
		
		void load();
		void save();
		
		void addEventListener(SpotifyAuthEventListener* listener);
		void removeEventListener(SpotifyAuthEventListener* listener);
		
		const Options& getOptions() const;
		bool isLoggedIn() const;
		bool isSessionValid() const;
		bool hasStreamingScope() const;
		bool canRefreshSession() const;
		const Optional<SpotifySession>& getSession() const;
		
		static Promise<Optional<SpotifySession>> authenticate(AuthenticateOptions options);
		struct LoginOptions {
			Optional<bool> showDialog;
		};
		Promise<Optional<SpotifySession>> authenticate(LoginOptions options = LoginOptions());
		Promise<bool> login(LoginOptions options = LoginOptions());
		void loginWithSession(SpotifySession session);
		void logout();
		
		using RenewOptions = OAuthSessionManager::RenewOptions;
		Promise<bool> renewSession(RenewOptions options = RenewOptions{.retryUntilResponse=false});
		Promise<bool> renewSessionIfNeeded(RenewOptions options = RenewOptions{.retryUntilResponse=false});
		
		static Promise<SpotifySession> swapCodeForToken(String code, AuthenticateOptions options);
		
	private:
		static String buildOAuthAuthorizationHeader(String clientId, String clientSecret);
		
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
		
		Options options;
		OAuthSessionManager sessionMgr;
		
		LinkedList<SpotifyAuthEventListener*> listeners;
		std::mutex listenersMutex;
	};
}

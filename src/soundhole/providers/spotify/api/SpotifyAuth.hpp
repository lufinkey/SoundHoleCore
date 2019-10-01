//
//  SpotifyAuth.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <chrono>
#include <soundhole/common.hpp>
#include "SpotifySession.hpp"
#include "SpotifyAuthEventListener.hpp"

namespace sh {
	class SpotifyAuth {
	public:
		struct Options {
			String clientId;
			String redirectURL;
			ArrayList<String> scopes;
			String tokenSwapURL;
			String tokenRefreshURL;
			std::map<String,String> params;
			String sessionPersistKey;
			std::chrono::seconds tokenRefreshEarliness = std::chrono::seconds(300);
			
			String getWebAuthenticationURL(String state) const;
			
			bool hasTokenSwapURL() const;
			bool hasTokenRefreshURL() const;
		};
		
		SpotifyAuth(Options options);
		~SpotifyAuth();
		
		void load();
		void save();
		
		void addEventListener(SpotifyAuthEventListener* listener);
		void removeEventListener(SpotifyAuthEventListener* listener);
		
		const Options& getOptions() const;
		bool isLoggedIn() const;
		bool isSessionValid() const;
		bool hasStreamingScope() const;
		bool canRefreshSession() const;
		
		struct LoginOptions {
			bool showDialog = true;
		};
		Promise<bool> login(LoginOptions options = LoginOptions{.showDialog=true});
		void loginWithSession(SpotifySession session);
		void logout();
		
		const Optional<SpotifySession>& getSession() const;
		
		struct RenewOptions {
			bool retryUntilResponse = false;
		};
		Promise<bool> renewSession(RenewOptions options = RenewOptions{.retryUntilResponse=false});
		Promise<bool> renewSessionIfNeeded(RenewOptions options = RenewOptions{.retryUntilResponse=false});
		
		static Promise<SpotifySession> swapCodeForToken(String code, String url);
		
	private:
		void startSession(SpotifySession session);
		void updateSession(String accessToken, SpotifySession::TimePoint expireTime);
		void clearSession();
		
		Promise<bool> performSessionRenewal();
		static Promise<Json> performTokenURLRequest(String url, std::map<String,String> params);
		
		void startRenewalTimer();
		void rescheduleRenewalTimer();
		void stopRenewalTimer();
		void onRenewalTimerFire();
		
		Options options;
		Optional<SpotifySession> session;
		std::recursive_mutex sessionMutex;
		
		struct WaitCallback {
			Promise<bool>::Resolver resolve;
			Promise<bool>::Rejecter reject;
		};
		struct RenewalInfo {
			bool deleted = false;
			LinkedList<WaitCallback> callbacks;
			LinkedList<WaitCallback> retryUntilResponseCallbacks;
		};
		std::shared_ptr<RenewalInfo> renewalInfo;
		
		LinkedList<SpotifyAuthEventListener*> listeners;
		std::mutex listenersMutex;
		
		SharedTimer renewalTimer;
	};
}

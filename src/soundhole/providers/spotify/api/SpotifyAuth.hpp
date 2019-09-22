//
//  SpotifyAuth.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "SpotifySession.hpp"

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
			
			String getWebAuthenticationURL(String state) const;
			
			bool hasTokenSwapURL() const;
			bool hasTokenRefreshURL() const;
		};
		
		SpotifyAuth(Options options);
		
		bool isLoggedIn() const;
		bool isSessionValid() const;
		bool hasStreamingScope() const;
		bool canRefreshSession() const;
		
		void load();
		void save();
		
		void startSession(SpotifySession session);
		void clearSession();
		
		struct RenewOptions {
			bool retryUntilResponse = false;
		};
		Promise<bool> renewSession(RenewOptions options = RenewOptions{.retryUntilResponse=false});
		Promise<bool> renewSessionIfNeeded(RenewOptions options = RenewOptions{.retryUntilResponse=false});
		
		static Promise<SpotifySession> swapCodeForToken(String code, String url);
		
	private:
		Promise<bool> performSessionRenewal();
		static Promise<Json> performTokenURLRequest(String url, std::map<String,String> params);
		
		Options options;
		Optional<SpotifySession> session;
		
		struct RenewCallback {
			Promise<bool>::Resolver resolve;
			Promise<bool>::Rejecter reject;
		};
		struct RenewalInfo {
			LinkedList<RenewCallback> callbacks;
			LinkedList<RenewCallback> retryUntilResponseCallbacks;
		};
		std::shared_ptr<RenewalInfo> renewalInfo;
		std::mutex renewalInfoMutex;
	};
}

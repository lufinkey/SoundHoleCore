//
//  OAuthSessionManager.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/13/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "OAuthSession.hpp"

namespace sh {
	class OAuthSessionManager {
	public:
		class Delegate {
		public:
			virtual ~Delegate() {}
			
			virtual String getOAuthSessionPersistKey(const OAuthSessionManager* mgr) const = 0;
			virtual std::map<String,String> getOAuthTokenRefreshParams(const OAuthSessionManager* mgr) const = 0;
			virtual std::map<String,String> getOAuthTokenRefreshHeaders(const OAuthSessionManager* mgr) const = 0;
			virtual std::chrono::seconds getOAuthTokenRefreshEarliness(const OAuthSessionManager* mgr) const = 0;
			virtual String getOAuthTokenRefreshURL(const OAuthSessionManager* mgr) const = 0;
			
			virtual void onOAuthSessionStart(OAuthSessionManager* mgr) {}
			virtual void onOAuthSessionResume(OAuthSessionManager* mgr) {}
			virtual void onOAuthSessionRenew(OAuthSessionManager* mgr) {}
			virtual void onOAuthSessionExpire(OAuthSessionManager* mgr) {}
			virtual void onOAuthSessionEnd(OAuthSessionManager* mgr) {}
		};
		
		OAuthSessionManager(Delegate*);
		~OAuthSessionManager();
		
		void load();
		void save();
		
		void startSession(OAuthSession);
		void endSession();
		
		const Optional<OAuthSession>& getSession() const;
		bool isSessionValid() const;
		bool canRefreshSession() const;
		
		struct RenewOptions {
			bool retryUntilResponse = false;
		};
		Promise<bool> renewSession(RenewOptions options);
		Promise<bool> renewSessionIfNeeded(RenewOptions options);
		
	private:
		void updateSession(OAuthSession);
		
		void startRenewalTimer();
		void rescheduleRenewalTimer();
		void stopRenewalTimer();
		void onRenewalTimerFire();
		
		void performSessionRenewal();
		
		Delegate* delegate;
		
		Optional<OAuthSession> session;
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
		
		SharedTimer renewalTimer;
	};
}

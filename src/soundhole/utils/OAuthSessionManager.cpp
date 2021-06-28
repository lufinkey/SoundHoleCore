//
//  OAuthSessionManager.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/13/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "OAuthSessionManager.hpp"

namespace sh {
	OAuthSessionManager::OAuthSessionManager(Delegate* delegate)
	: delegate(delegate) {
		//
	}

	OAuthSessionManager::~OAuthSessionManager() {
		if(renewalInfo != nullptr) {
			renewalInfo->deleted = true;
		}
	}



	void OAuthSessionManager::load() {
		auto sessionPersistKey = delegate->getOAuthSessionPersistKey(this);
		if(sessionPersistKey.empty()) {
			return;
		}
		bool hadSession = session.hasValue();
		std::unique_lock<std::recursive_mutex> lock(sessionMutex);
		session = OAuthSession::load(sessionPersistKey);
		lock.unlock();
		if(!hadSession && session) {
			delegate->onOAuthSessionResume(this);
			if(session && !session->isValid()) {
				delegate->onOAuthSessionExpire(this);
			}
		}
		renewSessionIfNeeded({.retryUntilResponse = true});
	}

	void OAuthSessionManager::save() {
		auto sessionPersistKey = delegate->getOAuthSessionPersistKey(this);
		if(sessionPersistKey.empty()) {
			return;
		}
		std::unique_lock<std::recursive_mutex> lock(sessionMutex);
		OAuthSession::save(sessionPersistKey, session);
	}



	void OAuthSessionManager::startSession(OAuthSession newSession) {
		std::unique_lock<std::recursive_mutex> lock(sessionMutex);
		bool hadSession = session.hasValue();
		session = newSession;
		save();
		if(session) {
			startRenewalTimer();
		}
		if(!hadSession && session) {
			delegate->onOAuthSessionStart(this);
			if(session && !session->isValid()) {
				delegate->onOAuthSessionExpire(this);
			}
		}
		renewSessionIfNeeded({.retryUntilResponse = true});
	}

	void OAuthSessionManager::updateSession(OAuthSession newSession) {
		std::unique_lock<std::recursive_mutex> lock(sessionMutex);
		if(!session) {
			startSession(newSession);
			return;
		}
		session = newSession;
		save();
		rescheduleRenewalTimer();
	}

	void OAuthSessionManager::endSession() {
		bool hadSession = session.hasValue();
		std::unique_lock<std::recursive_mutex> lock(sessionMutex);
		stopRenewalTimer();
		session = std::nullopt;
		save();
		if(hadSession) {
			delegate->onOAuthSessionEnd(this);
		}
	}



	const Optional<OAuthSession>& OAuthSessionManager::getSession() const {
		return session;
	}

	bool OAuthSessionManager::isSessionValid() const {
		return session.hasValue() && session->isValid();
	}

	bool OAuthSessionManager::canRefreshSession() const {
		return session.hasValue() && session->hasRefreshToken() && !delegate->getOAuthTokenRefreshURL(this).empty();
	}



	Promise<bool> OAuthSessionManager::renewSession(RenewOptions options) {
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
			
			if(shouldPerformRenewal) {
				performSessionRenewal();
			}
		});
	}


	Promise<bool> OAuthSessionManager::renewSessionIfNeeded(RenewOptions options) {
		if(!session || isSessionValid()) {
			// not logged in or session does not need renewal
			return Promise<bool>::resolve(false);
		} else if(!canRefreshSession()) {
			// no refresh token to renew session with, so the session has expired
			return Promise<bool>::reject(OAuthError(OAuthError::Code::SESSION_EXPIRED, "Session has expired"));
		} else {
			return renewSession(options);
		}
	}


	void OAuthSessionManager::performSessionRenewal() {
		std::unique_lock<std::recursive_mutex> lock(sessionMutex);
		// ensure session can be refreshed
		auto oldSession = this->session;
		auto tokenRefreshURL = delegate->getOAuthTokenRefreshURL(this);
		auto tokenRefreshParams = delegate->getOAuthTokenRefreshParams(this);
		auto tokenRefreshHeaders = delegate->getOAuthTokenRefreshHeaders(this);
		if(!canRefreshSession() || tokenRefreshURL.empty() || !oldSession) {
			// session has ended or cannot be refreshed, so call all callbacks with false and return
			std::shared_ptr<RenewalInfo> renewalInfo;
			renewalInfo.swap(this->renewalInfo);
			lock.unlock();
			for(auto& callback : renewalInfo->callbacks) {
				callback.resolve(false);
			}
			for(auto& callback : renewalInfo->retryUntilResponseCallbacks) {
				callback.resolve(false);
			}
			return;
		}
		// save renewalInfo pointer
		FGL_ASSERT(this->renewalInfo != nullptr, "renewalInfo must be set when performSessionRenewal is called");
		auto renewalInfo = this->renewalInfo;
		lock.unlock();
		// perform session renewal request
		OAuthSession::refreshSession(tokenRefreshURL, oldSession.value(), tokenRefreshParams, tokenRefreshHeaders)
		.then([=](OAuthSession newSession) {
			// function to call for cancelling session renewal
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
			// ensure object still exists before locking session mutex
			if(renewalInfo->deleted) {
				// object deleted, so call all callbacks with false and return
				cancelRenewal();
				return;
			}
			std::unique_lock<std::recursive_mutex> lock(this->sessionMutex);
			if(!canRefreshSession()) {
				// session ended or cannot be refreshed, so call all callbacks with false and return
				cancelRenewal();
				return;
			}
			
			// update session
			this->updateSession(newSession);
			
			// unset renewal info
			this->renewalInfo = nullptr;
			lock.unlock();
			
			// call renewal callbacks
			for(auto& callback : renewalInfo->callbacks) {
				callback.resolve(true);
			}
			for(auto& callback : renewalInfo->retryUntilResponseCallbacks) {
				callback.resolve(true);
			}
			
			// call delegate
			delegate->onOAuthSessionRenew(this);
			// done, renewal success
			
		}).except([=](std::exception_ptr errorPtr) {
			// ensure object still exists
			if(renewalInfo->deleted) {
				// object was deleted, so cancel session renewal
				for(auto& callback : renewalInfo->callbacks) {
					callback.reject(errorPtr);
				}
				for(auto& callback : renewalInfo->retryUntilResponseCallbacks) {
					callback.reject(errorPtr);
				}
				return;
			}
			// get renewal callbacks
			bool shouldRetry = false;
			LinkedList<WaitCallback> callbacks;
			std::unique_lock<std::recursive_mutex> lock(sessionMutex);
			callbacks.swap(renewalInfo->callbacks);
			try {
				std::rethrow_exception(errorPtr);
			} catch(OAuthError& error) {
				// if the request wasn't sent, we should try again if needed, otherwise give up and forward the error
				if(error.getCode() == OAuthError::Code::REQUEST_NOT_SENT && renewalInfo->retryUntilResponseCallbacks.size() > 0) {
					shouldRetry = true;
				}
			} catch(...) {
				// just forward the other errors
			}
			if(!shouldRetry) {
				this->renewalInfo = nullptr;
				callbacks.splice(callbacks.end(), renewalInfo->retryUntilResponseCallbacks);
			}
			lock.unlock();
			
			// call renewal callbacks
			for(auto& callback : callbacks) {
				callback.reject(errorPtr);
			}
			
			// retry if needed, otherwise fail
			if(shouldRetry) {
				// wait 2 seconds before retrying
				defaultPromiseQueue()->asyncAfter(std::chrono::steady_clock::now() + std::chrono::seconds(2), [=]() {
					if(renewalInfo->deleted) {
						// call callbacks
						for(auto& callback : renewalInfo->callbacks) {
							callback.resolve(false);
						}
						for(auto& callback : renewalInfo->retryUntilResponseCallbacks) {
							callback.resolve(false);
						}
						return;
					}
					this->performSessionRenewal();
				});
			}
		});
	}




	void OAuthSessionManager::startRenewalTimer() {
		if(renewalTimer && renewalTimer->isValid()) {
			// auth renewal timer has already been started, don't bother starting again
			return;
		}
		rescheduleRenewalTimer();
	}

	void OAuthSessionManager::rescheduleRenewalTimer() {
		if(!canRefreshSession()) {
			// we can't perform token refresh, so don't bother scheduling the timer
			return;
		}
		auto tokenRefreshEarliness = delegate->getOAuthTokenRefreshEarliness(this);
		auto now = std::chrono::system_clock::now();
		auto expireTime = session->getExpireTime();
		auto timeDiff = expireTime - now;
		auto renewalTimeDiff = (expireTime - tokenRefreshEarliness) - now;
		if(timeDiff <= std::chrono::seconds(30) || timeDiff <= (tokenRefreshEarliness + std::chrono::seconds(30)) || renewalTimeDiff <= std::chrono::seconds(0)) {
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

	void OAuthSessionManager::stopRenewalTimer() {
		if(renewalTimer) {
			renewalTimer->cancel();
			renewalTimer = nullptr;
		}
	}

	void OAuthSessionManager::onRenewalTimerFire() {
		renewSession({.retryUntilResponse=true});
	}
}

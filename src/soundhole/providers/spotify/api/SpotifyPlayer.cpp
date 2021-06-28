//
//  SpotifyPlayer.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/22/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SpotifyPlayer.hpp"

namespace sh {
	SpotifyPlayer* sharedSpotifyPlayer = nullptr;

	SpotifyPlayer* SpotifyPlayer::shared() {
		if(sharedSpotifyPlayer == nullptr) {
			sharedSpotifyPlayer = new SpotifyPlayer();
		}
		return sharedSpotifyPlayer;
	}


	void SpotifyPlayer::addEventListener(SpotifyPlayerEventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		listeners.pushBack(listener);
		lock.unlock();
	}

	void SpotifyPlayer::removeEventListener(SpotifyPlayerEventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		listeners.removeLastEqual(listener);
		lock.unlock();
	}


	void SpotifyPlayer::setOptions(Options options) {
		this->options = options;
	}

	const SpotifyPlayer::Options& SpotifyPlayer::getOptions() const {
		return options;
	}

	
	void SpotifyPlayer::setAuth(SpotifyAuth* auth) {
		std::unique_lock<std::mutex> lock(loginMutex);
		if(this->auth != nullptr) {
			this->auth->removeEventListener(this);
		}
		this->auth = auth;
		if(this->auth != nullptr) {
			this->auth->addEventListener(this);
		}
		lock.unlock();
		
		if(isStarted()) {
			if(auth != nullptr && auth->isLoggedIn()) {
				prepareForCall();
			} else {
				logout();
			}
		}
	}
	
	SpotifyAuth* SpotifyPlayer::getAuth() {
		return auth;
	}
	
	const SpotifyAuth* SpotifyPlayer::getAuth() const {
		return auth;
	}
	
	
	
	Promise<bool> SpotifyPlayer::startIfAble() {
		if(auth == nullptr) {
			return Promise<bool>::resolve(false);
		}
		return start().map<bool>([=]() { return true; });
	}
	
	
	
	bool SpotifyPlayer::isLoggedIn() const {
		return loggedIn;
	}
	
	Promise<bool> SpotifyPlayer::loginIfAble() {
		if(auth == nullptr || !auth->isLoggedIn()) {
			return Promise<bool>::resolve(false);
		}
		return login().map<bool>([]() { return true; });
	}
	
	Promise<void> SpotifyPlayer::login() {
		std::unique_lock<std::mutex> lock(loginMutex);
		if(auth == nullptr) {
			return Promise<void>::reject(SpotifyError(SpotifyError::Code::NOT_LOGGED_IN, "Auth has not been assigned to player"));
		} else if(!auth->isLoggedIn()) {
			return Promise<void>::reject(SpotifyError(SpotifyError::Code::NOT_LOGGED_IN, "Auth is not logged in"));
		}
		String accessToken = auth->getSession()->getAccessToken();
		std::unique_ptr<Promise<void>> resultPromise;
		if(loggingIn) {
			return Promise<void>([=](auto resolve, auto reject) {
				loginCallbacks.pushBack({ resolve, reject });
			});
		}
		else if(isPlayerLoggedIn()) {
			return Promise<void>::resolve();
		}
		else {
			resultPromise = std::make_unique<Promise<void>>(Promise<void>([=](auto resolve, auto reject) {
				loginCallbacks.pushBack({ resolve, reject });
			}));
		}
		loggingIn = true;
		lock.unlock();
		try {
			applyAuthToken(accessToken);
		} catch(...) {
			lock.lock();
			LinkedList<WaitCallback> callbacks;
			callbacks.swap(loginCallbacks);
			lock.unlock();
			auto error = std::current_exception();
			for(auto callback : callbacks) {
				callback.reject(error);
			}
		}
		return std::move(*resultPromise.get());
	}
	
	
	
	void SpotifyPlayer::renewSession(String accessToken) {
		// backup player state before we renew, because the spotify SDK is broken and logs you out
		std::unique_lock<std::mutex> lock(loginMutex);
		renewingSession = true;
		#ifdef TARGETPLATFORM_IOS
		renewingPlayerState = getPlayerState();
		#endif
		lock.unlock();
		applyAuthToken(accessToken);
		// TODO make this asyncAfter cancellable
		defaultPromiseQueue()->asyncAfter(std::chrono::steady_clock::now() + std::chrono::milliseconds(500), [=]() {
			std::unique_lock<std::mutex> lock(loginMutex);
			renewingSession = false;
			#ifdef TARGETPLATFORM_IOS
			renewingPlayerState = std::nullopt;
			#endif
		});
	}
	
	
	
	Promise<void> SpotifyPlayer::prepareForCall(bool condition) {
		if(!condition || auth == nullptr) {
			return Promise<void>::resolve();
		}
		return startIfAble().then([=](bool started) -> Promise<bool> {
			if(!started) {
				return Promise<bool>::resolve(false);
			}
			return loginIfAble();
		}).toVoid();
	}
	
	
	
	void SpotifyPlayer::beginSession() {
		std::unique_lock<std::mutex> lock(loginMutex);
		bool wasLoggedIn = loggedIn;
		loggingIn = false;
		loggedIn = true;
		LinkedList<WaitCallback> loginCallbacks;
		loginCallbacks.swap(this->loginCallbacks);
		lock.unlock();
		
		for(auto& callback : loginCallbacks) {
			callback.resolve();
		}
		
		// send login event if needed
		if(!wasLoggedIn) {
			//[self sendEvent:@"login" args:@[]];
		}
	}
	
	void SpotifyPlayer::endSession() {
		std::unique_lock<std::mutex> lock(loginMutex);
		bool wasLoggedIn = loggedIn;
		// clear session and stop player
		loggedIn = false;
		loggingIn = false;
		loggingOut = false;
		renewingSession = false;
		#ifdef TARGETPLATFORM_IOS
		renewingPlayerState = std::nullopt;
		#endif
		LinkedList<WaitCallback> logoutCallbacks;
		logoutCallbacks.swap(this->logoutCallbacks);
		LinkedList<WaitCallback> loginCallbacks;
		loginCallbacks.swap(this->loginCallbacks);
		lock.unlock();
		
		stop();
		
		// handle login callbacks
		for(auto& callback : loginCallbacks) {
			callback.reject(SpotifyError(SpotifyError::Code::NOT_LOGGED_IN, "You have been logged out"));
		}
		// handle logout callbacks
		for(auto& callback : logoutCallbacks) {
			callback.resolve();
		}
		
		// send logout event if needed
		if(wasLoggedIn) {
			//[self sendEvent:@"logout" args:@[]];
		}
	}
	
	
	
	
	void SpotifyPlayer::onStreamingLoginError(SpotifyError error) {
		std::unique_lock<std::mutex> lock(loginMutex);
		if(loggingIn) {
			loggingIn = false;
			// if the error is one that requires logging out, log out
			//bool sendLogoutEvent = false;
			if(loggedIn) {
				endSession();
				//sendLogoutEvent = true;
			}
			
			// handle loginCallbacks
			LinkedList<WaitCallback> loginCallbacks;
			loginCallbacks.swap(this->loginCallbacks);
			lock.unlock();
			
			for(auto& callback : loginCallbacks) {
				callback.reject(error);
			}
			
			/*if(sendLogoutEvent) {
				[self sendEvent:@"logout" args:@[]];
			}*/
		}
	}
	
	void SpotifyPlayer::onStreamingLogin() {
		// begin session / handle loginPlayer callbacks
		beginSession();
	}
	
	void SpotifyPlayer::onStreamingLogout() {
		std::unique_lock<std::mutex> lock(loginMutex);
		bool wasLoggingOut = loggingOut;
		loggingIn = false;
		loggingOut = false;
		
		// get loginCallbacks
		LinkedList<WaitCallback> loginCallbacks;
		loginCallbacks.swap(this->loginCallbacks);
		
		// get session info
		auto session = (auth != nullptr) ? auth->getSession() : Optional<SpotifySession>();
		auto tokenRefreshEarliness = (auth != nullptr) ? auth->getOptions().tokenRefreshEarliness : std::chrono::seconds(300);
		
		lock.unlock();
		
		// handle loginCallbacks
		for(auto& callback : loginCallbacks) {
			callback.reject(SpotifyError(SpotifyError::Code::NOT_LOGGED_IN, "You have been logged out"));
		}
		
		// we're intentionally logging out, or we don't have auth anymore
		if(wasLoggingOut || auth == nullptr || !auth->canRefreshSession() || !session) {
			// end player session
			endSession();
		}
		// if we didn't explicitly log out, try to renew the session
		else {
			// the player gets logged out when the session gets renewed (for some reason) so reuse access token
			if(renewingSession) {
				#ifdef TARGETPLATFORM_IOS
				auto savedPlayerState = this->renewingPlayerState;
				renewingPlayerState = std::nullopt;
				#endif
				renewingSession = false;
				if((session->getExpireTime() - std::chrono::system_clock::now()) > (tokenRefreshEarliness + std::chrono::seconds(300))) {
					login().then([=]() {
						#ifdef TARGETPLATFORM_IOS
						// reset back to saved player state if we have one
						if(savedPlayerState) {
							restoreSavedPlayerState(savedPlayerState.value());
						}
						#endif
					}).except([=](std::exception_ptr errorPtr) {
						// we failed to login the player, so end the player session
						endSession();
					});
					return;
				}
			}
			// renew session
			auth->renewSession({.retryUntilResponse = true}).then([=](bool renewed) {
				if(!renewed) {
					// we failed to renew, so end player session
					endSession();
				} else {
					// we renewed the auth token, so we're good here
				}
			}).except([=](std::exception_ptr errorPtr) {
				// we failed to renew, so end player session
				endSession();
			});
		}
	}

	void SpotifyPlayer::onStreamingError(SpotifyError error) {
		// TODO handle normal streaming error
	}
	
	
	
	void SpotifyPlayer::onSpotifyAuthSessionResume(SpotifyAuth* auth) {
		if(auth->isSessionValid()) {
			login();
		}
	}
	
	void SpotifyPlayer::onSpotifyAuthSessionStart(SpotifyAuth* auth) {
		if(auth->isSessionValid()) {
			login();
		}
	}
	
	void SpotifyPlayer::onSpotifyAuthSessionRenew(SpotifyAuth* auth) {
		if(auth->isSessionValid()) {
			renewSession(auth->getSession()->getAccessToken());
		}
	}
	
	void SpotifyPlayer::onSpotifyAuthSessionEnd(SpotifyAuth* auth) {
		logout();
	}
}

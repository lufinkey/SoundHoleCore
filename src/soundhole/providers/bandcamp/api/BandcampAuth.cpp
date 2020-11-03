//
//  BandcampAuth.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "BandcampAuth.hpp"

namespace sh {
	BandcampAuth::BandcampAuth(Options options)
	: options(options) {
		//
	}

	BandcampAuth::~BandcampAuth() {
		//
	}

	void BandcampAuth::load() {
		if(options.sessionPersistKey.empty()) {
			return;
		}
		bool wasLoggedIn = isLoggedIn();
		std::unique_lock<std::recursive_mutex> lock(sessionMutex);
		session = BandcampSession::load(options.sessionPersistKey);
		lock.unlock();
		if(!wasLoggedIn && session) {
			std::unique_lock<std::mutex> lock(listenersMutex);
			LinkedList<BandcampAuthEventListener*> listeners = this->listeners;
			lock.unlock();
			for(auto listener : listeners) {
				listener->onBandcampAuthSessionResume(this);
			}
		}
	}

	void BandcampAuth::save() {
		if(options.sessionPersistKey.empty()) {
			return;
		}
		std::unique_lock<std::recursive_mutex> lock(sessionMutex);
		BandcampSession::save(options.sessionPersistKey, session);
	}


	void BandcampAuth::addEventListener(BandcampAuthEventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		listeners.pushBack(listener);
	}

	void BandcampAuth::removeEventListener(BandcampAuthEventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		listeners.removeLastEqual(listener);
	}


	const BandcampAuth::Options& BandcampAuth::getOptions() const {
		return options;
	}

	bool BandcampAuth::isLoggedIn() const {
		return session.hasValue();
	}

	const Optional<BandcampSession>& BandcampAuth::getSession() const {
		return session;
	}



	void BandcampAuth::startSession(BandcampSession session) {
		std::unique_lock<std::recursive_mutex> lock(sessionMutex);
		this->session = session;
		save();
	}

	void BandcampAuth::loginWithSession(BandcampSession session) {
		bool wasLoggedIn = isLoggedIn();
		startSession(session);
		if(!wasLoggedIn && this->session) {
			std::unique_lock<std::mutex> lock(listenersMutex);
			LinkedList<BandcampAuthEventListener*> listeners = this->listeners;
			lock.unlock();
			for(auto listener : listeners) {
				listener->onBandcampAuthSessionStart(this);
			}
		}
	}

	void BandcampAuth::logout() {
		bool wasLoggedIn = isLoggedIn();
		clearSession();
		if(wasLoggedIn) {
			std::unique_lock<std::mutex> lock(listenersMutex);
			LinkedList<BandcampAuthEventListener*> listeners = this->listeners;
			lock.unlock();
			for(auto listener : listeners) {
				listener->onBandcampAuthSessionEnd(this);
			}
		}
	}

	void BandcampAuth::clearSession() {
		std::unique_lock<std::recursive_mutex> lock(sessionMutex);
		session.reset();
		save();
	}
}

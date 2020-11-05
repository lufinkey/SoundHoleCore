//
//  BandcampAuth.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include <napi.h>
#include "BandcampAuth.hpp"
#include <soundhole/scripts/Scripts.hpp>
#include <embed/nodejs/NodeJS.hpp>

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



	Promise<bool> BandcampAuth::login() {
		return authenticate().map<bool>([=](auto session) -> bool {
			if(!session) {
				return false;
			}
			loginWithSession(session.value());
			return true;
		});
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



	void BandcampAuth::startSession(BandcampSession session) {
		std::unique_lock<std::recursive_mutex> lock(sessionMutex);
		this->session = session;
		save();
	}

	void BandcampAuth::clearSession() {
		std::unique_lock<std::recursive_mutex> lock(sessionMutex);
		session.reset();
		save();
	}



	Promise<Optional<BandcampSession>> BandcampAuth::createBandcampSessionFromCookies(ArrayList<String> cookies) {
		return Promise<Optional<BandcampSession>>([=](auto resolve, auto reject) {
			scripts::loadScriptsIfNeeded().then(nullptr, [=]() {
				try {
					embed::nodejs::queueMain([=](napi_env env) {
						auto jsExports = scripts::getJSExports(env);
						
						// create cookies array
						auto cookiesArray = Napi::Array::New(env);
						auto cookiesArrayPush = cookiesArray.Get("push").As<Napi::Function>();
						for(auto& cookie : cookies) {
							cookiesArrayPush.Call(cookiesArray, { Napi::String::New(env, cookie.c_str()) });
						}
						
						// create session object
						auto BandcampClass = jsExports.Get("Bandcamp").As<Napi::Function>();
						auto SessionClass = BandcampClass.Get("Session").As<Napi::Function>();
						auto sessionJs = SessionClass.New({ cookiesArray });
						
						// check if session is logged in
						auto isLoggedIn = sessionJs.Get("isLoggedIn").As<Napi::Boolean>().Value();
						if(!isLoggedIn) {
							resolve(std::nullopt);
							return;
						}
						
						// serialize session
						auto serializeFunction = sessionJs.Get("serialize").As<Napi::Function>();
						auto sessionJsonStr = serializeFunction.Call(sessionJs, {}).As<Napi::String>().Utf8Value();
						std::string jsonError;
						auto sessionJson = Json::parse(sessionJsonStr, jsonError);
						if(sessionJson.is_null()) {
							printf("error parsing session json: %s\n", jsonError.c_str());
							resolve(std::nullopt);
							return;
						}
						
						// create BandcampSession
						auto session = BandcampSession::fromJson(sessionJson);
						resolve(session);
					});
				} catch(...) {
					reject(std::current_exception());
				}
			}, reject);
		});
	}
}

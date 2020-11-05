//
//  BandcampAuth.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/utils/js/JSWrapClass.hpp>
#include "BandcampSession.hpp"
#include "BandcampAuthEventListener.h"

namespace sh {
	class BandcampAuth {
	public:
		struct Options {
			String sessionPersistKey;
		};
		
		BandcampAuth(Options options);
		~BandcampAuth();
		
		void load();
		void save();
		
		void addEventListener(BandcampAuthEventListener* listener);
		void removeEventListener(BandcampAuthEventListener* listener);
		
		const Options& getOptions() const;
		bool isLoggedIn() const;
		const Optional<BandcampSession>& getSession() const;
		
		static Promise<Optional<BandcampSession>> authenticate();
		
		Promise<bool> login();
		void loginWithSession(BandcampSession session);
		void logout();
		
	private:
		void startSession(BandcampSession session);
		void clearSession();
		
		static Promise<Optional<BandcampSession>> createBandcampSessionFromCookies(ArrayList<String> cookies);
		
		Options options;
		Optional<BandcampSession> session;
		std::recursive_mutex sessionMutex;
		
		LinkedList<BandcampAuthEventListener*> listeners;
		std::mutex listenersMutex;
	};
}

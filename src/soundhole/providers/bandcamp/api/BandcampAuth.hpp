//
//  BandcampAuth.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
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
		
		Promise<bool> login();
		void loginWithSession(BandcampSession session);
		void logout();
		
	private:
		void startSession(BandcampSession session);
		void clearSession();
		
		Options options;
		Optional<BandcampSession> session;
		std::recursive_mutex sessionMutex;
		
		LinkedList<BandcampAuthEventListener*> listeners;
		std::mutex listenersMutex;
	};
}

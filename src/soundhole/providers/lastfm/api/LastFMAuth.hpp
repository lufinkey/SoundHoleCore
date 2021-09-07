//
//  LastFMAuth.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/6/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "LastFMTypes.hpp"

namespace sh {
	class LastFMAuth {
	public:
		struct Options {
			LastFMAPICredentials apiCredentials;
			String sessionPersistKey;
		};
		
		struct APIConfig {
			String apiRoot;
			String webLogin;
			String webSessionCookieName;
			Function<bool(String)> checkSessionCookieLoggedIn;
			
			String webLoginBaseURL() const;
			String webLoginHost() const;
			String webLoginBaseHost() const;
		};
		
		LastFMAuth(Options options, APIConfig apiConfig);
		
		const APIConfig& apiConfig() const;
		const LastFMAPICredentials& apiCredentials() const;
		Optional<LastFMSession> session() const;
		bool isLoggedIn() const;
		bool isWebLoggedIn() const;
		
		void load();
		void save();
		
		Promise<Optional<LastFMWebAuthResult>> authenticateWeb(Function<Promise<void>(LastFMWebAuthResult)> beforeDismiss = nullptr) const;
		Promise<LastFMSession> getMobileSession(String username, String password) const;
		
		Promise<Optional<Tuple<LastFMSession,LastFMWebAuthResult>>> authenticate() const;
		Promise<bool> login();
		void logout();
		
	private:
		void applyWebAuthResult(LastFMWebAuthResult);
		
		Options _options;
		APIConfig _apiConfig;
		Optional<LastFMSession> _session;
	};
}

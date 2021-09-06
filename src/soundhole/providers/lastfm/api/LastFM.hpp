//
//  LastFM.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/5/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "LastFMSession.hpp"
#include <soundhole/utils/HttpClient.hpp>

namespace sh {
	class LastFM {
	public:
		struct Options {
			String apiKey;
			String apiSecret;
			String sessionPersistKey;
		};
		LastFM(Options);
		
		Promise<Json> sendRequest(utils::HttpMethod httpMethod, String apiMethod, std::map<String,String> params = {}, bool usesAuth = true);
		
		Promise<LastFMSession> authenticate(String username, String password);
		Promise<void> login(String username, String password);
		void logout();
		
		Optional<LastFMSession> session() const;
		
	private:
		Optional<LastFMSession> loadSession();
		void saveSession();
		
		Options options;
		Optional<LastFMSession> _session;
	};
}

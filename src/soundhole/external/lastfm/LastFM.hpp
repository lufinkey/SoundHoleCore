//
//  LastFM.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/5/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "LastFMAuth.hpp"
#include "LastFMTypes.hpp"
#include <soundhole/utils/HttpClient.hpp>

namespace sh {
	class LastFM {
	public:
		struct Options {
			LastFMAuth::Options auth;
		};
		LastFM(Options);
		~LastFM();
		
		Promise<Json> sendRequest(utils::HttpMethod httpMethod, String apiMethod, std::map<String,String> params = {}, bool usesAuth = true);
		
		Promise<bool> login();
		void logout();
		
		bool isLoggedIn() const;
		
		Promise<LastFMScrobbleResponse> scrobble(LastFMScrobbleRequest request);
		
		Promise<void> loveTrack(String track, String artist);
		Promise<void> unloveTrack(String track, String artist);
		
	private:
		LastFMAuth* auth;
	};
}

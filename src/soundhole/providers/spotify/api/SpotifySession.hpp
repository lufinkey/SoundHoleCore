//
//  SpotifySession.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/external.hpp>
#include <chrono>

namespace sh {
	class SpotifySession {
	public:
		using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
		
		SpotifySession(String accessToken, TimePoint expireTime, String refreshToken, ArrayList<String> scopes);
		
		const String& getAccessToken() const;
		const TimePoint& getTimePoint() const;
		const String& getRefreshToken() const;
		const ArrayList<String>& getScopes() const;
		
		bool isValid() const;
		bool hasRefreshToken() const;
		bool hasScope(String scope) const;
		
		static TimePoint getExpireTimeFromSeconds(int seconds);
		
	private:
		String accessToken;
		TimePoint expireTime;
		String refreshToken;
		ArrayList<String> scopes;
	};
}

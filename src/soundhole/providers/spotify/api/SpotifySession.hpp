//
//  SpotifySession.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <chrono>
#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <Foundation/Foundation.h>
#endif

namespace sh {
	class SpotifySession {
	public:
		using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
		
		SpotifySession(String accessToken, TimePoint expireTime, String refreshToken, ArrayList<String> scopes);
		
		const String& getAccessToken() const;
		const TimePoint& getExpireTime() const;
		const String& getRefreshToken() const;
		const ArrayList<String>& getScopes() const;
		
		bool isValid() const;
		bool hasRefreshToken() const;
		bool hasScope(String scope) const;
		
		void update(String accessToken, TimePoint expireTime);
		
		static TimePoint getExpireTimeFromSeconds(int seconds);
		
		#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
		NSDictionary* toNSDictionary() const;
		void writeToNSUserDefaults(const String& key, NSUserDefaults* userDefaults) const;
		static void writeToNSUserDefaults(const Optional<SpotifySession>& session, const String& key, NSUserDefaults* userDefaults);
		
		static Optional<SpotifySession> fromNSDictionary(NSDictionary* dictionary);
		static Optional<SpotifySession> fromNSUserDefaults(const String& key, NSUserDefaults* userDefaults);
		#endif
		
	private:
		String accessToken;
		TimePoint expireTime;
		String refreshToken;
		ArrayList<String> scopes;
	};
}

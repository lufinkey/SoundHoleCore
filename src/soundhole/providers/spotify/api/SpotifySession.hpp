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
		
		static Optional<SpotifySession> load(const String& key);
		static void save(const String& key, Optional<SpotifySession> session);
		
		SpotifySession(String accessToken, TimePoint expireTime, String refreshToken, ArrayList<String> scopes);
		
		void save(const String& key);
		
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
		#if defined(JNIEXPORT) && defined(TARGETPLATFORM_ANDROID)
		void writeToAndroidSharedPrefs(JNIEnv* env, const String& key, jobject context) const;
		static void writeToAndroidSharedPrefs(JNIEnv* env, const Optional<SpotifySession>& session, const String& key, jobject context);

		static Optional<SpotifySession> fromAndroidSharedPrefs(JNIEnv* env, const String& key, jobject context);
		#endif
		
	private:
		String accessToken;
		TimePoint expireTime;
		String refreshToken;
		ArrayList<String> scopes;
	};
}

//
//  OAuthSession.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/13/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "OAuthError.hpp"

namespace sh {
	class OAuthSession {
	public:
		using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
		
		static Optional<OAuthSession> load(const String& key);
		static void save(const String& key, Optional<OAuthSession> session);
		
		OAuthSession(String accessToken, TimePoint expireTime, String tokenType, String refreshToken, ArrayList<String> scopes);
		
		void save(const String& key);
		
		const String& getAccessToken() const;
		const TimePoint& getExpireTime() const;
		const String& getTokenType() const;
		const String& getRefreshToken() const;
		const ArrayList<String>& getScopes() const;
		
		bool isValid() const;
		bool hasRefreshToken() const;
		bool hasScope(String scope) const;
		
		void update(String accessToken, TimePoint expireTime);
		
		static TimePoint getExpireTimeFromSeconds(int seconds);
		
		#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
		NSDictionary* toNSDictionary() const;
		NSDictionary* toJsonNSDictionary() const;
		void writeToNSUserDefaults(const String& key, NSUserDefaults* userDefaults) const;
		static void writeToNSUserDefaults(const Optional<OAuthSession>& session, const String& key, NSUserDefaults* userDefaults);
		void writeToKeychain(const String& key) const;
		static void writeToKeychain(const Optional<OAuthSession>& session, const String& key);
		
		static Optional<OAuthSession> fromNSDictionary(NSDictionary* dictionary);
		static Optional<OAuthSession> fromNSUserDefaults(const String& key, NSUserDefaults* userDefaults);
		static Optional<OAuthSession> fromKeychain(const String& key);
		#endif
		
		#if defined(JNIEXPORT) && defined(__ANDROID__)
		void writeToAndroidSharedPrefs(JNIEnv* env, const String& key, jobject context) const;
		static void writeToAndroidSharedPrefs(JNIEnv* env, const Optional<OAuthSession>& session, const String& key, jobject context);

		static Optional<OAuthSession> fromAndroidSharedPrefs(JNIEnv* env, const String& key, jobject context);
		#endif
		
		
		static Promise<Json> performTokenRequest(String url, std::map<String,String> params, std::map<String,String> headers);
		static Promise<OAuthSession> swapCodeForToken(String url, String code, std::map<String,String> params, std::map<String,String> headers);
		static Promise<OAuthSession> refreshSession(String url, OAuthSession session, std::map<String,String> params, std::map<String,String> headers);
		
	private:
		String accessToken;
		TimePoint expireTime;
		String tokenType;
		String refreshToken;
		ArrayList<String> scopes;
	};
}

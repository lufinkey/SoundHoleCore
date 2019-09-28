//
//  SpotifySession_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 9/22/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SpotifySession.hpp"

#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <Foundation/Foundation.h>

namespace sh {
	Optional<SpotifySession> SpotifySession::load(const String& key) {
		return SpotifySession::fromNSUserDefaults(key, NSUserDefaults.standardUserDefaults);
	}
	
	void SpotifySession::save(const String& key) {
		writeToNSUserDefaults(key, NSUserDefaults.standardUserDefaults);
	}
	
	NSDictionary* SpotifySession::toNSDictionary() const {
		NSMutableDictionary* dictionary = [NSMutableDictionary dictionary];
		dictionary[@"accessToken"] = accessToken.toNSString();
		dictionary[@"expireDate"] = [NSDate dateWithTimeIntervalSince1970:(NSTimeInterval)std::chrono::system_clock::to_time_t(expireTime)];
		if(refreshToken.size() > 0) {
			dictionary[@"refreshToken"] = refreshToken.toNSString();
		}
		NSMutableArray* scopesArr = [NSMutableArray array];
		for(auto& scope : scopes) {
			[scopesArr addObject:scope.toNSString()];
		}
		dictionary[@"scopes"] = scopesArr;
		return dictionary;
	}
	
	void SpotifySession::writeToNSUserDefaults(const String& key, NSUserDefaults* userDefaults) const {
		NSDictionary* dictionary = toNSDictionary();
		[userDefaults setObject:dictionary forKey:key.toNSString()];
	}
	
	void SpotifySession::writeToNSUserDefaults(const Optional<SpotifySession>& session, const String& key, NSUserDefaults* userDefaults) {
		if(session) {
			session->writeToNSUserDefaults(key, userDefaults);
		} else {
			[userDefaults removeObjectForKey:key.toNSString()];
		}
	}
	
	
	
	Optional<SpotifySession> SpotifySession::fromNSDictionary(NSDictionary* dictionary) {
		NSString* accessToken = dictionary[@"accessToken"];
		NSDate* expireDate = dictionary[@"expireDate"];
		NSString* refreshToken = dictionary[@"refreshToken"];
		NSArray* scopesArr = dictionary[@"scopes"];
		if(accessToken == nil || ![accessToken isKindOfClass:NSString.class]) {
			return {};
		} else if(expireDate == nil || ![expireDate isKindOfClass:NSDate.class]) {
			return {};
		} else if(refreshToken != nil && ![refreshToken isKindOfClass:NSString.class]) {
			return {};
		} else if(scopesArr != nil && ![scopesArr isKindOfClass:NSArray.class]) {
			return {};
		}
		if(refreshToken == nil) {
			refreshToken = @"";
		}
		TimePoint expireTime = std::chrono::system_clock::from_time_t((time_t)expireDate.timeIntervalSince1970);
		ArrayList<String> scopes;
		if(scopesArr != nil) {
			scopes.reserve(scopesArr.count);
			for(NSString* scope in scopesArr) {
				scopes.pushBack(String(scope));
			}
		}
		return SpotifySession(String(accessToken), expireTime, String(refreshToken), scopes);
	}
	
	Optional<SpotifySession> SpotifySession::fromNSUserDefaults(const String& key, NSUserDefaults* userDefaults) {
		NSDictionary* sessionData = [userDefaults objectForKey:key.toNSString()];
		if(sessionData == nil || ![sessionData isKindOfClass:[NSDictionary class]]) {
			return {};
		}
		return fromNSDictionary(sessionData);
	}
}

#endif

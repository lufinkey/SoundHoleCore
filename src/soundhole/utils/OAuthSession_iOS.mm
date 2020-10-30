//
//  OAuthSession_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 9/13/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "OAuthSession.hpp"

#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <Foundation/Foundation.h>
#import "SHKeychainUtils_ObjC.h"

namespace sh {
	Optional<OAuthSession> OAuthSession::load(const String& key) {
		return OAuthSession::fromKeychain(key);
	}

	void OAuthSession::save(const String& key, Optional<OAuthSession> session) {
		writeToKeychain(session, key);
	}
	
	void OAuthSession::save(const String& key) {
		writeToKeychain(key);
	}
	
	NSDictionary* OAuthSession::toNSDictionary() const {
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
	
	void OAuthSession::writeToNSUserDefaults(const String& key, NSUserDefaults* userDefaults) const {
		NSDictionary* dictionary = toNSDictionary();
		[userDefaults setObject:dictionary forKey:key.toNSString()];
	}
	
	void OAuthSession::writeToNSUserDefaults(const Optional<OAuthSession>& session, const String& key, NSUserDefaults* userDefaults) {
		if(session) {
			session->writeToNSUserDefaults(key, userDefaults);
		} else {
			[userDefaults removeObjectForKey:key.toNSString()];
		}
	}

	void OAuthSession::writeToKeychain(const String& key) const {
		NSDictionary* dictionary = toNSDictionary();
		NSError* error = nil;
		NSData* data = [NSJSONSerialization dataWithJSONObject:dictionary options:0 error:&error];
		if(error != nil) {
			printf("error encoding JSON for keychain: %s\n", error.localizedDescription.UTF8String);
			return;
		}
		[SHKeychainUtils setGenericPasswordData:data forAccountKey:key.toNSString()];
	}

	void OAuthSession::writeToKeychain(const Optional<OAuthSession>& session, const String& key) {
		if(session) {
			session->writeToKeychain(key);
		} else {
			[SHKeychainUtils deleteGenericPasswordDataForAccountKey:key.toNSString()];
		}
	}
	
	
	
	Optional<OAuthSession> OAuthSession::fromNSDictionary(NSDictionary* dictionary) {
		NSString* accessToken = dictionary[@"accessToken"];
		NSDate* expireDate = dictionary[@"expireDate"];
		NSString* refreshToken = dictionary[@"refreshToken"];
		NSArray* scopesArr = dictionary[@"scopes"];
		if(accessToken == nil || ![accessToken isKindOfClass:NSString.class]) {
			return std::nullopt;
		} else if(expireDate == nil || ![expireDate isKindOfClass:NSDate.class]) {
			return std::nullopt;
		} else if(refreshToken != nil && ![refreshToken isKindOfClass:NSString.class]) {
			return std::nullopt;
		} else if(scopesArr != nil && ![scopesArr isKindOfClass:NSArray.class]) {
			return std::nullopt;
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
		return OAuthSession(String(accessToken), expireTime, String(refreshToken), scopes);
	}
	
	Optional<OAuthSession> OAuthSession::fromNSUserDefaults(const String& key, NSUserDefaults* userDefaults) {
		NSDictionary* sessionData = [userDefaults objectForKey:key.toNSString()];
		if(sessionData == nil || ![sessionData isKindOfClass:[NSDictionary class]]) {
			return std::nullopt;
		}
		return fromNSDictionary(sessionData);
	}

	Optional<OAuthSession> OAuthSession::fromKeychain(const String& key) {
		NSData* data = [SHKeychainUtils genericPasswordDataForAccountKey:key.toNSString()];
		if(data == nil) {
			return std::nullopt;
		}
		NSError* error = nil;
		NSDictionary* dict = [NSJSONSerialization JSONObjectWithData:data options:0 error:&error];
		if(error != nil) {
			printf("error decoding JSON from keychain: %s\n", error.localizedDescription.UTF8String);
			return std::nullopt;
		}
		if(dict == nil || ![dict isKindOfClass:NSDictionary.class]) {
			return std::nullopt;
		}
		return fromNSDictionary(dict);
	}
}

#endif

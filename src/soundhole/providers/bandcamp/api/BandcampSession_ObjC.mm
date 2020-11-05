//
//  BandcampSession_ObjC.m
//  SoundHoleCore
//
//  Created by Luis Finke on 11/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "BandcampSession.hpp"
#include <soundhole/utils/HttpClient.hpp>

#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <Foundation/Foundation.h>
#import "SHKeychainUtils_ObjC.h"

namespace sh {
	Optional<BandcampSession> BandcampSession::load(const String& key) {
		return BandcampSession::fromKeychain(key);
	}

	void BandcampSession::save(const String& key, Optional<BandcampSession> session) {
		writeToKeychain(session, key);
	}
	
	void BandcampSession::save(const String& key) {
		writeToKeychain(key);
	}
	
	void BandcampSession::writeToKeychain(const String& key) const {
		String sessionStr = toJson().dump();
		NSData* sessionData = [NSData dataWithBytes:(const char*)sessionStr.c_str() length:(NSUInteger)sessionStr.length()];
		[SHKeychainUtils setGenericPasswordData:sessionData forAccountKey:key.toNSString()];
	}

	void BandcampSession::writeToKeychain(const Optional<BandcampSession>& session, const String& key) {
		if(session) {
			session->writeToKeychain(key);
		} else {
			[SHKeychainUtils deleteGenericPasswordDataForAccountKey:key.toNSString()];
		}
	}
	
	Optional<BandcampSession> BandcampSession::fromKeychain(const String& key) {
		NSData* data = [SHKeychainUtils genericPasswordDataForAccountKey:key.toNSString()];
		if(data == nil) {
			return std::nullopt;
		}
		String sessionStr = String((const char*)data.bytes, (size_t)data.length);
		std::string parseError;
		Json sessionJson = Json::parse(sessionStr.c_str(), parseError);
		if(sessionJson.is_null()) {
			return std::nullopt;
		}
		return BandcampSession::fromJson(sessionJson);
	}
}

#endif


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
		String cookiesStr = utils::encodeCookies(cookies);
		NSData* cookiesData = [NSData dataWithBytes:(const char*)cookiesStr.c_str() length:(NSUInteger)cookiesStr.length()];
		[SHKeychainUtils setGenericPasswordData:cookiesData forAccountKey:key.toNSString()];
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
		String cookiesStr = String((const char*)data.bytes, (size_t)data.length);
		auto cookies = utils::parseCookies(cookiesStr);
		return BandcampSession::fromCookies(cookies);
	}
}

#endif


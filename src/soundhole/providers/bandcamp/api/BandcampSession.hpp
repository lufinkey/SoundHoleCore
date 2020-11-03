//
//  BandcampSession.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class BandcampSession {
	public:
		static Optional<BandcampSession> fromCookies(const std::map<String,String>& cookies);
		
		static Optional<BandcampSession> load(const String& key);
		static void save(const String& key, Optional<BandcampSession> session);
		
		void save(const String& key);
		
		String getClientId() const;
		String getIdentityString() const;
		String getSessionString() const;
		
		#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
		void writeToKeychain(const String& key) const;
		static void writeToKeychain(const Optional<BandcampSession>& session, const String& key);
		static Optional<BandcampSession> fromKeychain(const String& key);
		#endif
		
	private:
		BandcampSession(const std::map<String,String>& cookies);
		
		String getCookieValue(const String& key) const;
		
		std::map<String,String> cookies;
	};
}

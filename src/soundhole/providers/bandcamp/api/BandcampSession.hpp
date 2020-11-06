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
		static Optional<BandcampSession> fromJson(const Json&);
		
		static Optional<BandcampSession> load(const String& key);
		static void save(const String& key, Optional<BandcampSession> session);
		
		void save(const String& key);
		
		const String& getClientId() const;
		const String& getIdentity() const;
		const ArrayList<String>& getCookies() const;
		
		bool equals(const BandcampSession&) const;
		
		Json toJson() const;
		
		#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
		void writeToKeychain(const String& key) const;
		static void writeToKeychain(const Optional<BandcampSession>& session, const String& key);
		static Optional<BandcampSession> fromKeychain(const String& key);
		#endif
		
	private:
		BandcampSession(String clientId, String identity, ArrayList<String> cookies);
		
		String clientId;
		String identity;
		ArrayList<String> cookies;
	};



	inline bool operator==(const sh::BandcampSession& left, const sh::BandcampSession& right) {
		return left.equals(right);
	}

	inline bool operator!=(const sh::BandcampSession& left, const sh::BandcampSession& right) {
		return !left.equals(right);
	}
}

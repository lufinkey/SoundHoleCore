//
//  BandcampSession.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "BandcampSession.hpp"

namespace sh {
	Optional<BandcampSession> BandcampSession::fromCookies(const std::map<String, String>& cookies) {
		auto session = BandcampSession(cookies);
		if(session.getClientId().empty() || session.getIdentityString().empty()) {
			return std::nullopt;
		}
		return session;
	}

	BandcampSession::BandcampSession(const std::map<String,String>& cookies)
	: cookies(cookies) {
		//
	}

	String BandcampSession::getCookieValue(const String& key) const {
		auto it = cookies.find(key);
		if(it == cookies.end()) {
			return String();
		}
		return it->second;
	}

	String BandcampSession::getClientId() const {
		return getCookieValue("client_id");
	}

	String BandcampSession::getIdentityString() const {
		return getCookieValue("identity");
	}

	String BandcampSession::getSessionString() const {
		return getCookieValue("session");
	}
}

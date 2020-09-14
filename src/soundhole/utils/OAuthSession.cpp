//
//  OAuthSession.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/13/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "OAuthSession.hpp"

namespace sh {
	OAuthSession::OAuthSession(String accessToken, TimePoint expireTime, String refreshToken, ArrayList<String> scopes)
	: accessToken(accessToken), expireTime(expireTime), refreshToken(refreshToken), scopes(scopes) {
		//
	}
	
	const String& OAuthSession::getAccessToken() const {
		return accessToken;
	}
	
	const OAuthSession::TimePoint& OAuthSession::getExpireTime() const {
		return expireTime;
	}
	
	const String& OAuthSession::getRefreshToken() const {
		return refreshToken;
	}
	
	const ArrayList<String>& OAuthSession::getScopes() const {
		return scopes;
	}
	
	bool OAuthSession::isValid() const {
		return (expireTime > std::chrono::system_clock::now());
	}
	
	bool OAuthSession::hasRefreshToken() const {
		return (refreshToken.length() > 0);
	}
	
	bool OAuthSession::hasScope(String scope) const {
		return scopes.contains(scope);
	}
	
	void OAuthSession::update(String accessToken, TimePoint expireTime) {
		this->accessToken = accessToken;
		this->expireTime = expireTime;
	}
	
	OAuthSession::TimePoint OAuthSession::getExpireTimeFromSeconds(int seconds) {
		return std::chrono::system_clock::now() + std::chrono::seconds(seconds);
	}
}

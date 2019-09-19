//
//  SpotifySession.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SpotifySession.hpp"

namespace sh {
	SpotifySession::SpotifySession(String accessToken, TimePoint expireTime, String refreshToken, ArrayList<String> scopes)
	: accessToken(accessToken), expireTime(expireTime), refreshToken(refreshToken), scopes(scopes) {
		//
	}
	
	const String& SpotifySession::getAccessToken() const {
		return accessToken;
	}
	
	const SpotifySession::TimePoint& SpotifySession::getTimePoint() const {
		return expireTime;
	}
	
	const String& SpotifySession::getRefreshToken() const {
		return refreshToken;
	}
	
	const ArrayList<String>& SpotifySession::getScopes() const {
		return scopes;
	}
	
	bool SpotifySession::isValid() const {
		return (expireTime > std::chrono::system_clock::now());
	}
	
	bool SpotifySession::hasRefreshToken() const {
		return (refreshToken.length() > 0);
	}
	
	bool SpotifySession::hasScope(String scope) const {
		return scopes.contains(scope);
	}
	
	SpotifySession::TimePoint SpotifySession::getExpireTimeFromSeconds(int seconds) {
		return std::chrono::system_clock::now() + std::chrono::seconds(seconds);
	}
}

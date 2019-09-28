//
//  SpotifyAuthEventListener.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/22/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class SpotifyAuth;
	class SpotifySession;
	
	class SpotifyAuthEventListener {
	public:
		virtual ~SpotifyAuthEventListener() {}
		
		virtual void onSpotifyAuthSessionResume(SpotifyAuth* auth) {}
		virtual void onSpotifyAuthSessionStart(SpotifyAuth* auth) {}
		virtual void onSpotifyAuthSessionRenew(SpotifyAuth* auth) {}
		virtual void onSpotifyAuthSessionExpire(SpotifyAuth* auth) {}
		virtual void onSpotifyAuthSessionEnd(SpotifyAuth* auth) {}
	};
}

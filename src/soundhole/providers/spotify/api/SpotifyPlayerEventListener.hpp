//
//  SpotifyPlayerEventListener.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/5/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "SpotifyPlaybackEvent.hpp"

namespace sh {
	class SpotifyPlayer;

	class SpotifyPlayerEventListener {
	public:
		virtual ~SpotifyPlayerEventListener() {}
		
		virtual void onSpotifyPlayerTemporaryConnectionError(SpotifyPlayer* player) {}
		virtual void onSpotifyPlayerMessage(SpotifyPlayer* player, String message) {}
		virtual void onSpotifyPlayerDisconnect(SpotifyPlayer* player) {}
		virtual void onSpotifyPlayerReconnect(SpotifyPlayer* player) {}
		virtual void onSpotifyPlaybackEvent(SpotifyPlayer* player, SpotifyPlaybackEvent event) {}
	};
}

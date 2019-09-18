//
//  SpotifyProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SpotifyProvider.hpp"

namespace sh {
	String SpotifyProvider::getName() const {
		return "spotify";
	}
	
	String SpotifyProvider::getDisplayName() const {
		return "Spotify";
	}
}

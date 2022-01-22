//
//  MediaMatcher.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/21/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#include "MediaMatcher.hpp"

namespace sh {
	$<OmniTrack> MediaMatcher::omniTrack(const OmniTrack::Data& data) {
		return OmniTrack::new$(this, data);
	}

	$<OmniArtist> MediaMatcher::omniArtist(const OmniArtist::Data& data) {
		return OmniArtist::new$(this, data);
	}
}

//
//  BandcampPlaybackProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/13/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "BandcampPlaybackProvider.hpp"
#include <soundhole/utils/SoundHoleError.hpp>

namespace sh {
	BandcampPlaybackProvider::BandcampPlaybackProvider(BandcampMediaProvider* provider, StreamPlayer* streamPlayer)
	: StreamPlaybackProvider(streamPlayer), provider(provider) {
		//
	}
}

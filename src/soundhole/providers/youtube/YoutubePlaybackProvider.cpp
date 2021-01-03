//
//  YoutubePlaybackProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/14/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "YoutubePlaybackProvider.hpp"
#include <soundhole/utils/SoundHoleError.hpp>

namespace sh {
	YoutubePlaybackProvider::YoutubePlaybackProvider(YoutubeMediaProvider* provider, StreamPlayer* streamPlayer)
	: StreamPlaybackProvider(streamPlayer), provider(provider) {
		//
	}
}

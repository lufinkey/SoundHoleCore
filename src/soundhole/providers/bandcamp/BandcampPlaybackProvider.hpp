//
//  BandcampPlaybackProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/13/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/playback/StreamPlaybackProvider.hpp>
#include "api/Bandcamp.hpp"

namespace sh {
	class BandcampProvider;

	class BandcampPlaybackProvider: public StreamPlaybackProvider {
	public:
		BandcampPlaybackProvider(BandcampProvider* provider, StreamPlayer* streamPlayer = StreamPlayer::shared());
		
	private:
		BandcampProvider* provider;
	};
}

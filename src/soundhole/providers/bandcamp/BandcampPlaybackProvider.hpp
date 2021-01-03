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
	class BandcampMediaProvider;

	class BandcampPlaybackProvider: public StreamPlaybackProvider {
	public:
		BandcampPlaybackProvider(BandcampMediaProvider* provider, StreamPlayer* streamPlayer = StreamPlayer::shared());
		
	private:
		BandcampMediaProvider* provider;
	};
}

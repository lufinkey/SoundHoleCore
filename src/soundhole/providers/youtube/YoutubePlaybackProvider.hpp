//
//  YoutubePlaybackProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/14/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/playback/StreamPlaybackProvider.hpp>
#include "api/Youtube.hpp"

namespace sh {
	class YoutubeMediaProvider;

	class YoutubePlaybackProvider: public StreamPlaybackProvider {
	public:
		YoutubePlaybackProvider(YoutubeMediaProvider* provider, StreamPlayer* player = StreamPlayer::shared());
		
	private:
		YoutubeMediaProvider* provider;
	};
}

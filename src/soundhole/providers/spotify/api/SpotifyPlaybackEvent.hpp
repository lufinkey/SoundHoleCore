//
//  SpotifyPlaybackEvent.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/5/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <Foundation/Foundation.h>
#import <SpotifyAudioPlayback/SpotifyAudioPlayback.h>
#endif

namespace sh {
	enum class SpotifyPlaybackEvent {
		PLAY,
		PAUSE,
		TRACK_CHANGE,
		METADATA_CHANGE,
		SHUFFLE_CHANGE,
		REPEAT_CHANGE,
		ACTIVE_CHANGE,
		LOST_PERMISSION,
		AUDIO_FLUSH,
		TRACK_DELIVERED,
		AUDIO_DELIVERY_DONE
	};

	#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
	SpotifyPlaybackEvent SpotifyPlaybackEvent_fromSpPlaybackEvent(SpPlaybackEvent);
	#endif
}

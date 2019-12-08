//
//  SpotifyPlaybackEvent_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 12/5/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SpotifyPlaybackEvent.hpp"

#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)

namespace sh {
	SpotifyPlaybackEvent SpotifyPlaybackEvent_fromSpPlaybackEvent(SpPlaybackEvent event) {
		#define EVENT_CASE(SpEvent, SpotifyEvent) \
			case SPPlaybackNotify##SpEvent: \
				return SpotifyPlaybackEvent::SpotifyEvent;
		switch(event) {
			EVENT_CASE(Play, PLAY)
			EVENT_CASE(Pause, PAUSE)
			EVENT_CASE(TrackChanged, TRACK_CHANGE)
			case SPPlaybackNotifyNext:
			case SPPlaybackNotifyPrev:
			case SPPlaybackNotifyContextChanged:
			EVENT_CASE(MetadataChanged, METADATA_CHANGE)
			EVENT_CASE(ShuffleOn, SHUFFLE_CHANGE)
			EVENT_CASE(ShuffleOff, SHUFFLE_CHANGE)
			EVENT_CASE(RepeatOn, REPEAT_CHANGE)
			EVENT_CASE(RepeatOff, REPEAT_CHANGE)
			EVENT_CASE(BecameActive, ACTIVE_CHANGE)
			EVENT_CASE(BecameInactive, ACTIVE_CHANGE)
			EVENT_CASE(LostPermission, LOST_PERMISSION)
			case SPPlaybackEventAudioFlush:
				return SpotifyPlaybackEvent::AUDIO_FLUSH;
			EVENT_CASE(TrackDelivered, TRACK_DELIVERED)
			EVENT_CASE(AudioDeliveryDone, AUDIO_DELIVERY_DONE)
		}
		#undef EVENT_CASE
	}
}

#endif

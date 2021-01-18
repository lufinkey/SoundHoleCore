//
//  MediaControls.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/18/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class MediaControls {
	public:
		enum class RepeatMode: uint8_t {
			OFF,
			ONE,
			ALL
		};
		
		enum class HandlerStatus {
			SUCCESS,
			FAILED,
			NO_SUCH_CONTENT,
			NO_NOW_PLAYING_ITEM,
			DEVICE_NOT_FOUND
		};
		
		class Listener {
		public:
			virtual ~Listener() {}
			
			virtual HandlerStatus onMediaControlsPause() = 0;
			virtual HandlerStatus onMediaControlsPlay() = 0;
			virtual HandlerStatus onMediaControlsStop() = 0;
			
			virtual HandlerStatus onMediaControlsPrevious() = 0;
			virtual HandlerStatus onMediaControlsNext() = 0;
			
			virtual HandlerStatus onMediaControlsChangeRepeatMode(RepeatMode) = 0;
			virtual HandlerStatus onMediaControlsChangeShuffleMode(bool) = 0;
		};
		
		virtual void addListener(Listener*) = 0;
		virtual void removeListener(Listener*) = 0;
		
		
		struct ButtonState {
			bool playEnabled = false;
			bool pauseEnabled = false;
			bool stopEnabled = false;
			bool previousEnabled = false;
			bool nextEnabled = false;
			bool repeatEnabled = false;
			RepeatMode repeatMode = RepeatMode::OFF;
			bool shuffleEnabled = false;
			bool shuffleMode = false;
		};
		
		virtual void setButtonState(ButtonState) = 0;
		virtual ButtonState getButtonState() const = 0;
		
		
		enum PlaybackState {
			PLAYING,
			PAUSED,
			STOPPED,
			UNKNOWN,
			INTERRUPTED
		};
		
		virtual void setPlaybackState(PlaybackState) = 0;
		virtual PlaybackState getPlaybackState() const = 0;
		
		
		struct NowPlaying {
			Optional<String> title;
			Optional<String> artist;
			Optional<String> albumTitle;
			Optional<size_t> trackIndex;
			Optional<size_t> trackCount;
			Optional<double> elapsedTime;
			Optional<double> duration;
		};
		
		virtual void setNowPlaying(NowPlaying) = 0;
		virtual NowPlaying getNowPlaying() const = 0;
	};
}

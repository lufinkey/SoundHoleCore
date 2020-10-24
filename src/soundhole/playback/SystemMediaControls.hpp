//
//  SystemMediaControls.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/23/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	struct _SystemMediaControlsNativeData;
	struct _SystemMediaControlsListenerNode;

	class SystemMediaControls {
	public:
		static SystemMediaControls* shared();
		
		enum class RepeatMode: uint8_t {
			OFF,
			ONE,
			ALL
		};
		
		class Listener {
		public:
			enum HandlerStatus {
				SUCCESS,
				FAILED,
				NO_SUCH_CONTENT,
				NO_NOW_PLAYING_ITEM,
				DEVICE_NOT_FOUND
			};
			
			virtual ~Listener() {}
			
			virtual HandlerStatus onSystemMediaControlsPause() = 0;
			virtual HandlerStatus onSystemMediaControlsPlay() = 0;
			virtual HandlerStatus onSystemMediaControlsStop() = 0;
			
			virtual HandlerStatus onSystemMediaControlsPrevious() = 0;
			virtual HandlerStatus onSystemMediaControlsNext() = 0;
			
			virtual HandlerStatus onSystemMediaControlsChangeRepeatMode(RepeatMode) = 0;
			virtual HandlerStatus onSystemMediaControlsChangeShuffleMode(bool) = 0;
		};
		
		~SystemMediaControls();
		
		void addListener(Listener*);
		void removeListener(Listener*);
		
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
		
		void setButtonState(ButtonState);
		ButtonState getButtonState() const;
		
		enum PlaybackState {
			PLAYING,
			PAUSED,
			STOPPED,
			UNKNOWN,
			INTERRUPTED
		};
		
		void setPlaybackState(PlaybackState);
		PlaybackState getPlaybackState() const;
		
		struct NowPlaying {
			Optional<String> title;
			Optional<String> artist;
			Optional<String> albumTitle;
			Optional<size_t> trackIndex;
			Optional<size_t> trackCount;
			Optional<double> elapsedTime;
			Optional<double> duration;
		};
		
		void setNowPlaying(NowPlaying);
		NowPlaying getNowPlaying() const;
		
	private:
		SystemMediaControls();
		SystemMediaControls(const SystemMediaControls&) = delete;
		SystemMediaControls& operator=(const SystemMediaControls&) = delete;
		
		_SystemMediaControlsNativeData* nativeData;
		LinkedList<_SystemMediaControlsListenerNode*> listenerNodes;
	};
}

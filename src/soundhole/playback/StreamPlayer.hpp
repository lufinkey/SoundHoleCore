//
//  StreamPlayer.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/8/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#ifdef __OBJC__
#import <Foundation/Foundation.h>
#endif
#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <AVFoundation/AVFoundation.h>
#include "StreamPlayerEventHandler_iOS.hpp"
#endif

namespace sh {
	class StreamPlayer {
	public:
		static StreamPlayer* shared();
		
		class Listener {
		public:
			virtual ~Listener() {}
			
			virtual void onStreamPlayerPlay(StreamPlayer* player) {}
			virtual void onStreamPlayerPause(StreamPlayer* player) {}
			virtual void onStreamPlayerTrackFinish(StreamPlayer* player, String audioURL) {}
		};
		
		struct PlaybackState {
			bool playing;
			double position;
			double duration;
		};
		
		StreamPlayer();
		~StreamPlayer();
		
		void addListener(Listener* listener);
		void removeListener(Listener* listener);
		
		Promise<void> prepare(String audioURL);
		struct PlayOptions {
			double position = 0;
			Function<void()> beforePlay;
		};
		Promise<void> play(String audioURL, PlayOptions options = PlayOptions{ .position = 0.0, .beforePlay = nullptr });
		Promise<void> setPlaying(bool playing);
		Promise<void> seek(double position);
		Promise<void> stop();
		
		PlaybackState getState() const;
		String getAudioURL() const;
		
	private:
		#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
		AVPlayer* createPlayer(String audioURL);
		void setPlayer(AVPlayer* player, String audioURL);
		#endif
		void destroyPlayer();
		void destroyPreparedPlayer();
		
		#ifdef TARGETPLATFORM_IOS
		OBJCPP_PTR(AVPlayer) player;
		OBJCPP_PTR(AVPlayer) preparedPlayer;
		#endif
		String playerAudioURL;
		String preparedAudioURL;
		mutable std::recursive_mutex playerMutex;
		
		OBJCPP_PTR(StreamPlayerEventHandler) playerEventHandler;
		
		AsyncQueue playQueue;
		
		LinkedList<Listener*> listeners;
		std::mutex listenersMutex;
	};
}

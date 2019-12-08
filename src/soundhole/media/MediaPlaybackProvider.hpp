//
//  MediaPlaybackProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/1/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "Track.hpp"

namespace sh {
	class MediaPlaybackProvider {
	public:
		struct State {
			bool playing;
			double position;
			bool shuffling;
			bool repeating;
		};
		
		struct Metadata {
			$<Track> previousTrack;
			$<Track> currentTrack;
			$<Track> nextTrack;
		};
		
		virtual ~MediaPlaybackProvider() {}
		
		virtual bool usesPublicAudioStreams() const = 0;
		
		virtual Promise<void> prepare($<Track> track) = 0;
		virtual Promise<void> play($<Track> track, double position) = 0;
		virtual Promise<void> setPlaying(bool playing) = 0;
		virtual void stop() = 0;
		virtual Promise<void> seek(double position) = 0;
		
		virtual State state() const = 0;
		virtual Metadata metadata() const = 0;
	};
}

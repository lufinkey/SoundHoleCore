//
//  BandcampPlaybackProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/13/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/MediaPlaybackProvider.hpp>
#include <soundhole/playback/StreamPlayer.hpp>
#include "api/Bandcamp.hpp"

namespace sh {
	class BandcampProvider;

	class BandcampPlaybackProvider: public MediaPlaybackProvider, protected StreamPlayer::Listener {
	public:
		BandcampPlaybackProvider(BandcampProvider* provider, StreamPlayer* player = StreamPlayer::shared());
		~BandcampPlaybackProvider();
		
		virtual bool usesPublicAudioStreams() const override;
		
		virtual Promise<void> prepare($<Track> track) override;
		virtual Promise<void> play($<Track> track, double position) override;
		virtual Promise<void> setPlaying(bool playing) override;
		virtual void stop() override;
		virtual Promise<void> seek(double position) override;
		
		virtual State state() const override;
		virtual Metadata metadata() const override;
		
	protected:
		virtual void onStreamPlayerPlay(StreamPlayer* player) override;
		virtual void onStreamPlayerPause(StreamPlayer* player) override;
		virtual void onStreamPlayerTrackFinish(StreamPlayer* player, String audioURL) override;
		
	private:
		BandcampProvider* provider;
		StreamPlayer* player;
		
		$<Track> currentTrack;
		String currentTrackAudioURL;
		mutable std::mutex currentTrackMutex;
		
		AsyncQueue prepareQueue;
		AsyncQueue playQueue;
	};
}

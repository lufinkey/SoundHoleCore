//
//  StreamPlaybackProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/14/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/MediaPlaybackProvider.hpp>
#include "StreamPlayer.hpp"

namespace sh {
	class StreamPlaybackProvider: public MediaPlaybackProvider, protected StreamPlayer::Listener {
	public:
		StreamPlaybackProvider(StreamPlayer* player);
		virtual ~StreamPlaybackProvider();
		
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
		StreamPlayer* player;
		
		$<Track> currentTrack;
		String currentTrackAudioURL;
		mutable std::mutex currentTrackMutex;
		
		AsyncQueue prepareQueue;
		AsyncQueue playQueue;
	};
}

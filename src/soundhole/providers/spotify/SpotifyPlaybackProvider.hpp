//
//  SpotifyPlaybackProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/1/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/MediaPlaybackProvider.hpp>
#include "api/Spotify.hpp"

namespace sh {
	class SpotifyProvider;

	class SpotifyPlaybackProvider: public MediaPlaybackProvider, protected SpotifyPlayerEventListener {
	public:
		SpotifyPlaybackProvider(SpotifyProvider* provider);
		~SpotifyPlaybackProvider();
		
		virtual bool usesPublicAudioStreams() const override;
		
		virtual Promise<void> prepare($<Track> track) override;
		virtual Promise<void> play($<Track> track, double position) override;
		virtual Promise<void> setPlaying(bool playing) override;
		virtual void stop() override;
		virtual Promise<void> seek(double position) override;
		
		virtual State state() const override;
		virtual Metadata metadata() const override;
		
	protected:
		Track::Data createTrackData(SpotifyPlayer::Track track) const;
		
		virtual void onSpotifyPlaybackEvent(SpotifyPlayer* player, SpotifyPlaybackEvent event) override;
		
	private:
		Promise<void> setPlayingUntilSuccess();
		
		SpotifyProvider* provider;
		AsyncQueue playQueue;
		AsyncQueue setPlayingQueue;
	};
}

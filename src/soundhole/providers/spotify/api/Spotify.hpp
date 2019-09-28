//
//  Spotify.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/28/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/utils/HttpClient.hpp>
#include "SpotifyAuth.hpp"
#include "SpotifyPlayer.hpp"

namespace sh {
	class Spotify {
	public:
		using Options = SpotifyAuth::Options;
		
		Spotify(Options options);
		
		Promise<bool> login();
		Promise<void> logout();
		
		Promise<void> startPlayer();
		void stopPlayer();
		
		using PlayOptions = SpotifyPlayer::PlayOptions;
		Promise<void> playURI(String uri, PlayOptions options = {.index=0,.position=0});
		Promise<void> queueURI(String uri);
		
		Promise<void> skipToNext();
		Promise<void> skipToPrevious();
		Promise<void> seek(double position);
		
		Promise<void> setPlaying(bool playing);
		Promise<void> setShuffling(bool shuffling);
		Promise<void> setRepeating(bool repeating);
		
		SpotifyPlayer::State getPlaybackState() const;
		SpotifyPlayer::Metadata getPlaybackMetadata() const;
		
		Promise<Json> sendRequest(utils::HttpMethod method, String endpoint, Json params = Json());
		
	private:
		Promise<void> prepareForPlayer();
		Promise<void> prepareForRequest();
		
		SpotifyAuth* auth;
		SpotifyPlayer* player;
		std::mutex playerMutex;
	};
}

//
//  SpotifyPlaylistMutatorDelegate.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/15/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/Playlist.hpp>

namespace sh {
	class SpotifyProvider;

	class SpotifyPlaylistMutatorDelegate: public Playlist::MutatorDelegate {
	public:
		SpotifyPlaylistMutatorDelegate(SpotifyProvider* provider);
		
		virtual Promise<void> loadTrackCollectionItems($<TrackCollection> playlist, Mutator* mutator, size_t index, size_t count) override;
		
	private:
		SpotifyProvider* provider;
	};
}

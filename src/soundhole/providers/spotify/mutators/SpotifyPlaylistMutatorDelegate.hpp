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
		friend class SpotifyProvider;
	public:
		virtual size_t getChunkSize() const override;
		virtual Promise<void> loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) override;
		virtual Promise<void> insertItems(Mutator* mutator, size_t index, LinkedList<$<Track>> tracks) override;
		virtual Promise<void> appendItems(Mutator* mutator, LinkedList<$<Track>> tracks) override;
		virtual Promise<void> removeItems(Mutator* mutator, size_t index, size_t count) override;
		virtual Promise<void> moveItems(Mutator* mutator, size_t index, size_t count, size_t newIndex) override;
		
	private:
		SpotifyPlaylistMutatorDelegate($<Playlist> playlist);
		
		w$<Playlist> playlist;
	};
}

//
//  SpotifyAlbumMutatorDelegate.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/15/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/Album.hpp>

namespace sh {
	class SpotifyProvider;

	class SpotifyAlbumMutatorDelegate: public Album::MutatorDelegate {
	public:
		SpotifyAlbumMutatorDelegate($<Album> album);
		
		virtual Promise<void> loadItems(Mutator* mutator, size_t index, size_t count) override;
		
	private:
		w$<Album> album;
	};
}

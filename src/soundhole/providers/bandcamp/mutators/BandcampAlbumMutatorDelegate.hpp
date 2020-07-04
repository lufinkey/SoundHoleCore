//
//  BandcampAlbumMutatorDelegate.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/16/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/Album.hpp>

namespace sh {
	class BandcampProvider;

	class BandcampAlbumMutatorDelegate: public Album::MutatorDelegate {
		friend class BandcampProvider;
	public:
		virtual size_t getChunkSize() override;
		virtual Promise<void> loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) override;
		
	private:
		BandcampAlbumMutatorDelegate($<Album> album);
		
		w$<Album> album;
	};
}

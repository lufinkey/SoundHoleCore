//
//  BandcampAlbumMutatorDelegate.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/16/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "BandcampAlbumMutatorDelegate.hpp"
#include <soundhole/providers/spotify/SpotifyProvider.hpp>
#include <soundhole/utils/SoundHoleError.hpp>

namespace sh {
	BandcampAlbumMutatorDelegate::BandcampAlbumMutatorDelegate($<Album> album)
	: album(album) {
		//
	}

	Promise<void> BandcampAlbumMutatorDelegate::loadItems(Mutator* mutator, size_t index, size_t count) {
		auto album = this->album.lock();
		auto provider = (SpotifyProvider*)album->mediaProvider();
		return provider->getAlbumData(album->uri()).then([=](Album::Data albumData) {
			if(!albumData.tracks) {
				throw SoundHoleError(SoundHoleError::Code::REQUEST_FAILED, "Failed to get tracks for bandcamp album");
			}
			mutator->lock([&]() {
				mutator->resize(albumData.tracks->total);
				mutator->apply(albumData.tracks->offset, albumData.tracks->items.map<$<AlbumItem>>([&](auto& albumItem) {
					return AlbumItem::new$(album, albumItem);
				}));
			});
		});
	}
}

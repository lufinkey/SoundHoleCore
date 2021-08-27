//
//  BandcampAlbumMutatorDelegate.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/16/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "BandcampAlbumMutatorDelegate.hpp"
#include <soundhole/providers/bandcamp/BandcampMediaProvider.hpp>
#include <soundhole/database/MediaDatabase.hpp>
#include <soundhole/utils/SoundHoleError.hpp>

namespace sh {
	BandcampAlbumMutatorDelegate::BandcampAlbumMutatorDelegate($<Album> album)
	: album(album) {
		//
	}

	size_t BandcampAlbumMutatorDelegate::getChunkSize() const {
		return 50;
	}

	Promise<void> BandcampAlbumMutatorDelegate::loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) {
		auto album = this->album.lock();
		if(options.offline && options.database != nullptr) {
			// offline load
			return options.database->loadAlbumItems(album, mutator, index, count);
		} else {
			// online load
			auto provider = (BandcampMediaProvider*)album->mediaProvider();
			return provider->getAlbumData(album->uri()).then([=](Album::Data albumData) {
				mutator->lock([&]() {
					auto items = albumData.items.mapValues([&](auto index, auto& albumItem) -> $<AlbumItem> {
						return album->createCollectionItem(albumItem);
					});
					if(albumData.itemCount) {
						mutator->applyAndResize(albumData.itemCount.value(), items);
					} else {
						mutator->apply(items);
					}
				});
			});
		}
	}
}

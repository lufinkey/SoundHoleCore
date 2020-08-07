//
//  BandcampAlbumMutatorDelegate.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/16/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "BandcampAlbumMutatorDelegate.hpp"
#include <soundhole/providers/bandcamp/BandcampProvider.hpp>
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
		if(options.database == nullptr) {
			// online load
			auto provider = (BandcampProvider*)album->mediaProvider();
			return provider->getAlbumData(album->uri()).then([=](Album::Data albumData) {
				if(!albumData.tracks) {
					throw SoundHoleError(SoundHoleError::Code::REQUEST_FAILED, "Failed to get tracks for bandcamp album");
				}
				mutator->lock([&]() {
					mutator->applyAndResize(albumData.tracks->offset, albumData.tracks->total,
					   albumData.tracks->items.map<$<AlbumItem>>([&](auto& albumItem) {
						return AlbumItem::new$(album, albumItem);
					}));
				});
			});
		}
		else {
			// offline load
			return options.database->loadAlbumItems(album, mutator, index, count);
		}
	}


	Promise<void> BandcampAlbumMutatorDelegate::insertItems(Mutator* mutator, size_t index, LinkedList<$<Track>> tracks) {
		return Promise<void>::reject(std::runtime_error("Cannot insert items into an album"));
	}

	Promise<void> BandcampAlbumMutatorDelegate::appendItems(Mutator* mutator, LinkedList<$<Track>> tracks) {
		return Promise<void>::reject(std::runtime_error("Cannot append items to an album"));
	}

	Promise<void> BandcampAlbumMutatorDelegate::removeItems(Mutator* mutator, size_t index, size_t count) {
		return Promise<void>::reject(std::runtime_error("Cannot remove items from an album"));
	}

	Promise<void> BandcampAlbumMutatorDelegate::moveItems(Mutator* mutator, size_t index, size_t count, size_t newIndex) {
		return Promise<void>::reject(std::runtime_error("Cannot move items inside an album"));
	}
}

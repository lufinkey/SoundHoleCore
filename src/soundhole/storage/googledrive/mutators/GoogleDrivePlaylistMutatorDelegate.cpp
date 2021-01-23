//
//  GoogleDrivePlaylistMutatorDelegate.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/23/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "GoogleDrivePlaylistMutatorDelegate.hpp"
#include <soundhole/storage/googledrive/GoogleDriveStorageProvider.hpp>
#include <soundhole/database/MediaDatabase.hpp>

namespace sh {
	GoogleDrivePlaylistMutatorDelegate::GoogleDrivePlaylistMutatorDelegate($<Playlist> playlist, GoogleDriveStorageProvider* storageProvider)
	: playlist(playlist), storageProvider(storageProvider) {
		//
	}

	size_t GoogleDrivePlaylistMutatorDelegate::getChunkSize() const {
		return 100;
	}

	Promise<void> GoogleDrivePlaylistMutatorDelegate::loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) {
		auto playlist = this->playlist.lock();
		if(options.offline && options.database != nullptr) {
			// offline load
			return options.database->loadPlaylistItems(playlist, mutator, index, count);
		}
		else {
			// online load
			return storageProvider->getPlaylistItems(playlist->uri(), index, count)
			.then([=](GoogleDrivePlaylistItemsPage page) -> void {
				auto items = page.items.map<$<PlaylistItem>>([&](auto& playlistItemData) {
					return playlist->createCollectionItem(playlistItemData);
				});
				mutator->lock([&]() {
					mutator->applyAndResize(page.offset, page.total, items);
				});
			});
		}
	}


	Promise<void> GoogleDrivePlaylistMutatorDelegate::insertItems(Mutator* mutator, size_t index, LinkedList<$<Track>> tracks) {
		return Promise<void>::reject(std::runtime_error("not implemented"));
	}

	Promise<void> GoogleDrivePlaylistMutatorDelegate::appendItems(Mutator* mutator, LinkedList<$<Track>> tracks) {
		return Promise<void>::reject(std::runtime_error("not implemented"));
	}

	Promise<void> GoogleDrivePlaylistMutatorDelegate::removeItems(Mutator* mutator, size_t index, size_t count) {
		return Promise<void>::reject(std::runtime_error("not implemented"));
	}

	Promise<void> GoogleDrivePlaylistMutatorDelegate::moveItems(Mutator* mutator, size_t index, size_t count, size_t newIndex) {
		return Promise<void>::reject(std::runtime_error("not implemented"));
	}
}

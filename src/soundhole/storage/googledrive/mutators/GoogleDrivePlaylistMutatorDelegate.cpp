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


	
	Promise<void> GoogleDrivePlaylistMutatorDelegate::loadAPIItems(Mutator* mutator, size_t index, size_t count) {
		auto playlist = this->playlist.lock();
		return storageProvider->getPlaylistItems(playlist->uri(), index, count)
		.then([=](GoogleDrivePlaylistItemsPage page) -> void {
			auto items = page.items.map([&](auto& playlistItemData) -> $<PlaylistItem> {
				return playlist->createCollectionItem(playlistItemData);
			});
			mutator->lock([&]() {
				mutator->applyAndResize(page.offset, page.total, items);
			});
		});
	}



	Promise<void> GoogleDrivePlaylistMutatorDelegate::loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) {
		if(options.offline && options.database != nullptr) {
			// offline load
			auto playlist = this->playlist.lock();
			return options.database->loadPlaylistItems(playlist, mutator, index, count);
		}
		else {
			// online load
			return loadAPIItems(mutator, index, count);
		}
	}



	Promise<void> GoogleDrivePlaylistMutatorDelegate::insertItems(Mutator* mutator, size_t index, LinkedList<$<Track>> tracks, InsertItemOptions options) {
		size_t chunkSize = getChunkSize();
		size_t halfChunkSize = chunkSize / 2;
		
		auto playlist = this->playlist.lock();
		auto storageProvider = this->storageProvider;
		auto list = mutator->getList();
		
		auto indexMarker = list->watchIndex(index);
		
		size_t lowerBound = indexMarker->index;
		if(lowerBound > halfChunkSize) {
			lowerBound -= halfChunkSize;
		} else {
			lowerBound = 0;
		}
		
		// load track data before adding
		auto promises = tracks.map([=](auto& track) -> Promise<void> {
			return track->fetchDataIfNeeded();
		});
		// load items around index to make sure we have latest version
		promises.pushBack(loadAPIItems(mutator, lowerBound, chunkSize));
		
		return Promise<void>::all(ArrayList<Promise<void>>(promises))
		.finally([=]() {
			list->unwatchIndex(indexMarker);
		})
		.then([=]() {
			// add tracks to playlist
			return storageProvider->insertPlaylistItems(playlist->uri(), indexMarker->index, tracks);
		})
		.then([=](GoogleDrivePlaylistItemsPage page) {
			mutator->lock([&]() {
				mutator->insert(indexMarker->index, page.items.map([&](auto& item) -> $<PlaylistItem> {
					return playlist->createCollectionItem(item);
				}));
			});
		});
	}



	Promise<void> GoogleDrivePlaylistMutatorDelegate::appendItems(Mutator* mutator, LinkedList<$<Track>> tracks, InsertItemOptions options) {
		size_t chunkSize = getChunkSize();
		size_t halfChunkSize = chunkSize / 2;
		
		auto playlist = this->playlist.lock();
		auto storageProvider = this->storageProvider;
		auto list = mutator->getList();
		size_t origListSize = list->size().valueOr(0);
		size_t chunkStart = origListSize;
		if(chunkStart > halfChunkSize) {
			chunkStart -= halfChunkSize;
		} else {
			chunkStart = 0;
		}
		
		// load track data before adding
		auto promises = tracks.map([=](auto& track) -> Promise<void> {
			return track->fetchDataIfNeeded();
		});
		// load items in chunk to make sure we have the latest version
		promises.pushBack(loadAPIItems(mutator, chunkStart, chunkSize));
		
		return Promise<void>::all(ArrayList<Promise<void>>(promises))
		.then([=]() {
			size_t newListSize = list->size().valueOr(0);
			if(newListSize != origListSize) {
				if(newListSize < chunkStart || newListSize >= (chunkStart + chunkSize)) {
					size_t chunk2Start = newListSize;
					if(chunk2Start > halfChunkSize) {
						chunk2Start -= halfChunkSize;
					} else {
						chunk2Start = 0;
					}
					return loadItems(mutator, chunkStart, chunkSize, LoadItemOptions());
				}
			}
			return Promise<void>::resolve();
		})
		.then([=]() {
			// add tracks to playlist
			return storageProvider->appendPlaylistItems(playlist->uri(), tracks);
		})
		.then([=](GoogleDrivePlaylistItemsPage page) {
			mutator->lock([&]() {
				mutator->insert(page.offset, page.items.map([&](auto& item) -> $<PlaylistItem> {
					return playlist->createCollectionItem(item);
				}));
			});
		});
	}



	Promise<void> GoogleDrivePlaylistMutatorDelegate::removeItems(Mutator* mutator, size_t index, size_t count) {
		if(count == 0) {
			return Promise<void>::resolve();
		}
		
		auto playlist = this->playlist.lock();
		auto storageProvider = this->storageProvider;
		
		auto list = mutator->getList();
		auto items = list->getLoadedItems({
			.startIndex = index,
			.limit = count,
			.onlyValidItems = false
		});
		if(items.size() != count) {
			return Promise<void>::reject(
				std::invalid_argument("Missing item at index "+std::to_string(index+items.size())));
		}
		ArrayList<String> itemIds;
		itemIds.reserve(items.size());
		LinkedList<AsyncListIndexMarker> indexMarkersList;
		for(auto& item : items) {
			auto itemId = item->uniqueId();
			if(itemId.empty()) {
				return Promise<void>::reject(
					std::runtime_error("Missing uniqueId prop for item at index "+std::to_string(index+itemIds.size())));
			}
			itemIds.pushBack(itemId);
			auto indexMarker = list->watchIndex(index);
			indexMarkersList.pushBack(indexMarker);
		}
		
		return storageProvider->deletePlaylistItems(playlist->uri(), itemIds)
		.then([=](auto indexes) {
			auto indexMarkers = indexMarkersList;
			mutator->lock([&]() {
				indexMarkers.sort([&](auto& a, auto& b) -> bool {
					return (a->index <= b->index);
				});
				Optional<size_t> startIndex;
				Optional<size_t> endIndex;
				for(auto& indexMarker : reversed(indexMarkers)) {
					if(indexMarker->state == AsyncListIndexMarkerState::REMOVED) {
						size_t index = indexMarker->index;
						size_t count = 2;
						if(index > 0) {
							index--;
							count++;
						}
						mutator->invalidate(index, count);
						continue;
					}
					if(!startIndex) {
						// start removal group
						startIndex = indexMarker->index;
						endIndex = startIndex.value() + 1;
					}
					else if(indexMarker->index != (startIndex.value() - 1)) {
						// next index doesn't match expected index, so remove and start new group
						size_t newStartIndex = indexMarker->index;
						mutator->remove(startIndex.value(), (endIndex.value() - startIndex.value()));
						startIndex = newStartIndex;
						endIndex = startIndex.value() + 1;
					} else {
						// expand removal group
						startIndex = indexMarker->index;
					}
				}
				if(startIndex) {
					// delete group
					mutator->remove(startIndex.value(), (endIndex.value() - startIndex.value()));
				}
			});
		})
		.finally([=]() {
			for(auto& indexMarker : indexMarkersList) {
				list->unwatchIndex(indexMarker);
			}
		});
	}



	Promise<void> GoogleDrivePlaylistMutatorDelegate::moveItems(Mutator* mutator, size_t index, size_t count, size_t newIndex) {
		if(count == 0 || index == newIndex) {
			return Promise<void>::resolve();
		}
		size_t chunkSize = getChunkSize();
		size_t halfChunkSize = chunkSize / 2;
		size_t padding = halfChunkSize / 2;
		
		auto playlist = this->playlist.lock();
		auto storageProvider = this->storageProvider;
		auto list = mutator->getList();
		
		auto moveIndexes = fgl::new$<LinkedList<AsyncListIndexMarker>>();
		for(size_t i=0; i<count; i++) {
			moveIndexes->pushBack(list->watchIndex(index+i));
		}
		auto destIndexMarker = (newIndex <= index) ? list->watchRemovedIndex(newIndex) : list->watchRemovedIndex(newIndex+count);
		
		size_t lowerBound = index;
		if(lowerBound > padding) {
			lowerBound -= padding;
		} else {
			lowerBound = 0;
		}
		size_t upperBound = index + count + padding;
		
		return loadAPIItems(mutator, lowerBound, (upperBound - lowerBound))
		.then([=]() {
			moveIndexes->sort([](auto& a, auto& b) {
				return a->index <= b->index;
			});
			Optional<size_t> prevIndex;
			for(auto& indexMarker : *moveIndexes) {
				if(indexMarker->state == AsyncListIndexMarkerState::REMOVED) {
					throw std::runtime_error("item at index "+std::to_string(indexMarker->index)+" was removed");
				}
				auto item = list->itemAt(indexMarker->index).valueOr(nullptr);
				if(!item) {
					throw std::runtime_error("could not find a valid item at move index "+std::to_string(indexMarker->index));
				}
				if(!prevIndex || (prevIndex.value() + 1) == indexMarker->index) {
					prevIndex = indexMarker->index;
				} else {
					throw std::runtime_error("move indexes are not consecutive after preload");
				}
			}
			auto firstIndexMarker = moveIndexes->front();
			size_t newIndex = destIndexMarker->index;
			if(destIndexMarker->index > firstIndexMarker->index) {
				if((destIndexMarker->index - firstIndexMarker->index) < count) {
					newIndex = firstIndexMarker->index;
				} else {
					newIndex = destIndexMarker->index - count;
				}
			}
			return storageProvider->movePlaylistItems(playlist->uri(), firstIndexMarker->index, count, newIndex);
		})
		.then([=]() {
			mutator->lock([&]() {
				// sort move indexes
				moveIndexes->sort([](auto& a, auto& b) {
					return (a->index <= b->index);
				});
				// check if indexes have changed
				bool indexesChanged = false;
				Optional<size_t> prevIndex;
				for(auto& indexMarker : *moveIndexes) {
					if(indexMarker->state == AsyncListIndexMarkerState::REMOVED) {
						indexesChanged = true;
						continue;
					}
					if(!prevIndex || (prevIndex.value() + 1) == indexMarker->index) {
						prevIndex = indexMarker->index;
					} else {
						indexesChanged = true;
						break;
					}
				}
				
				// apply move
				auto firstIndexMarker = moveIndexes->front();
				size_t destIndex = destIndexMarker->index;
				if(destIndexMarker->index > firstIndexMarker->index) {
					if(destIndex >= count) {
						destIndex -= count;
					} else {
						destIndex = 0;
					}
				}
				size_t firstIndex = firstIndexMarker->index;
				auto lastIndexMarker = moveIndexes->back();
				size_t lastIndex = lastIndexMarker->index;
				if(prevIndex) {
					size_t moveCount = (prevIndex.value() + 1) - firstIndex;
					mutator->move(firstIndex, moveCount, destIndex);
				}
				
				// if indexes have changed, invalidate the ranges where that happened
				if(indexesChanged) {
					size_t invPadding = 10;
					
					size_t invalidateSourceRangeStart = firstIndex;
					size_t invalidateSourceRangeEnd = lastIndex + 1;
					if((invalidateSourceRangeEnd - invalidateSourceRangeStart) < count) {
						invalidateSourceRangeEnd = invalidateSourceRangeStart + count;
					}
					if(invalidateSourceRangeStart < invPadding) {
						invalidateSourceRangeStart = 0;
					} else {
						invalidateSourceRangeStart -= invPadding;
					}
					invalidateSourceRangeEnd += invPadding;
					
					size_t invalidateDestRangeStart = destIndex;
					size_t invalidateDestRangeEnd = destIndex+count;
					if(invalidateDestRangeStart < invPadding) {
						invalidateDestRangeStart = 0;
					} else {
						invalidateSourceRangeStart -= invPadding;
					}
					invalidateDestRangeEnd += invPadding;
					
					mutator->invalidate(invalidateSourceRangeStart, (invalidateSourceRangeEnd-invalidateSourceRangeStart));
					mutator->invalidate(invalidateDestRangeStart, (invalidateDestRangeEnd-invalidateDestRangeStart));
				}
			});
		})
		.finally([=]() {
			for(auto& indexMarker : *moveIndexes) {
				list->unwatchIndex(indexMarker);
			}
			list->unwatchIndex(destIndexMarker);
		});
	}
}

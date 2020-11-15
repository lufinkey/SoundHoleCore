//
//  SpotifyPlaylistMutatorDelegate.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/15/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "SpotifyPlaylistMutatorDelegate.hpp"
#include <soundhole/providers/spotify/SpotifyProvider.hpp>
#include <soundhole/database/MediaDatabase.hpp>

namespace sh {
	SpotifyPlaylistMutatorDelegate::SpotifyPlaylistMutatorDelegate($<Playlist> playlist)
	: playlist(playlist) {
		//
	}

	size_t SpotifyPlaylistMutatorDelegate::getChunkSize() const {
		return 100;
	}

	Promise<void> SpotifyPlaylistMutatorDelegate::loadAPIItems(Mutator* mutator, size_t index, size_t count) {
		auto playlist = this->playlist.lock();
		auto provider = (SpotifyProvider*)playlist->mediaProvider();
		auto uriParts = provider->parseURI(playlist->uri());
		return provider->spotify->getPlaylistTracks(uriParts.id, {
			.market="from_token",
			.offset=index,
			.limit=count
		}).then([=](SpotifyPage<SpotifyPlaylist::Item> page) -> void {
			auto items = page.items.map<$<PlaylistItem>>([&](auto& item) {
				return PlaylistItem::new$(playlist, provider->createPlaylistItemData(item));
			});
			mutator->lock([&]() {
				mutator->applyAndResize(page.offset, page.total, items);
			});
		});
	}

	Promise<void> SpotifyPlaylistMutatorDelegate::loadDatabaseItems(Mutator* mutator, MediaDatabase* db, size_t index, size_t count) {
		auto playlist = this->playlist.lock();
		return db->loadPlaylistItems(playlist, mutator, index, count);
	}

	Promise<SpotifyPage<SpotifyPlaylist::Item>> SpotifyPlaylistMutatorDelegate::fetchAPIItemsFromChunks(Mutator* mutator, size_t index, size_t count) {
		auto playlist = this->playlist.lock();
		auto list = mutator->getList();
		auto provider = (SpotifyProvider*)playlist->mediaProvider();
		auto uriParts = provider->parseURI(playlist->uri());
		
		size_t i = index;
		size_t endIndex = index + count;
		size_t chunkSize = getChunkSize();
		
		auto promise = Promise<void>::resolve();
		auto fetchedItems = fgl::new$<LinkedList<SpotifyPlaylist::Item>>();
		auto listSize = fgl::new$<size_t>(list->size().valueOr(0));
		auto hrefStr = fgl::new$<String>();
		auto previousStr = fgl::new$<String>();
		auto nextStr = fgl::new$<String>();
		while(i < endIndex) {
			promise = promise.then([=]() {
				return provider->spotify->getPlaylistTracks(uriParts.id, {
					.market="from_token",
					.offset=i,
					.limit=chunkSize
				}).then([=](SpotifyPage<SpotifyPlaylist::Item> page) -> void {
					fetchedItems->pushBackList(page.items);
					if(i == index) {
						*hrefStr = page.href;
						*previousStr = page.previous;
					}
					*nextStr = page.next;
					*listSize = page.total;
				});
			});
			i += chunkSize;
		}
		return promise.map<SpotifyPage<SpotifyPlaylist::Item>>([=]() {
			return SpotifyPage<SpotifyPlaylist::Item>{
				.href = std::move(*hrefStr),
				.limit = fetchedItems->size(),
				.offset = index,
				.total = *listSize,
				.previous = std::move(*previousStr),
				.next = std::move(*nextStr),
				.items = std::move(*fetchedItems)
			};
		});
	}

	Promise<void> SpotifyPlaylistMutatorDelegate::loadAPIItemsFromChunks(Mutator* mutator, size_t index, size_t count) {
		auto playlist = this->playlist.lock();
		auto provider = (SpotifyProvider*)playlist->mediaProvider();
		return fetchAPIItemsFromChunks(mutator, index, count).then([=](SpotifyPage<SpotifyPlaylist::Item> page) {
			auto items = page.items.map<$<PlaylistItem>>([&](auto& item) {
				return PlaylistItem::new$(playlist, provider->createPlaylistItemData(item));
			});
			mutator->lock([&]() {
				mutator->applyAndResize(index, page.total, items);
			});
		});
	}



	Promise<void> SpotifyPlaylistMutatorDelegate::loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) {
		if(options.offline && options.database != nullptr) {
			// offline load
			return loadDatabaseItems(mutator, options.database, index, count);
		}
		else {
			// online load
			return loadAPIItems(mutator, index, count);
		}
	}



	Promise<void> SpotifyPlaylistMutatorDelegate::insertItems(Mutator* mutator, size_t index, LinkedList<$<Track>> tracks) {
		size_t chunkSize = getChunkSize();
		size_t halfChunkSize = chunkSize / 2;
		
		auto playlist = this->playlist.lock();
		auto provider = (SpotifyProvider*)playlist->mediaProvider();
		auto uriParts = provider->parseURI(playlist->uri());
		auto list = mutator->getList();
		
		auto indexMarker = list->watchIndex(index);
		size_t lowerBound = indexMarker->index;
		if(lowerBound > halfChunkSize) {
			lowerBound -= halfChunkSize;
		} else {
			lowerBound = 0;
		}
		
		// load items around index to make sure we have latest version
		return loadAPIItems(mutator, lowerBound, chunkSize)
		.finally([=]() {
			list->unwatchIndex(indexMarker);
		})
		.then([=]() {
			// add tracks to playlist
			return provider->spotify->addPlaylistTracks(uriParts.id, tracks.map<String>([](auto& track) {
				return track->uri();
			}), {
				.position = indexMarker->index
			}).then([=](SpotifyPlaylist::AddResult addResult) {
				// update versionId
				auto data = playlist->toData({
					.tracksOffset = 0,
					.tracksLimit = 0
				});
				data.versionId = addResult.snapshotId;
				playlist->applyData(data);
			});
		})
		.then([=]() {
			// load range of added items to fetch them
			size_t lowerBound = indexMarker->index;
			size_t padding = halfChunkSize / 2;
			if(lowerBound > padding) {
				lowerBound -= padding;
			} else {
				lowerBound = 0;
			}
			size_t upperBound = indexMarker->index + tracks.size() + padding;
			size_t fetchCount = upperBound - lowerBound;
			return fetchAPIItemsFromChunks(mutator, lowerBound, fetchCount).then([=](SpotifyPage<SpotifyPlaylist::Item> page) {
				auto items = page.items.map<$<PlaylistItem>>([&](auto& item) {
					return PlaylistItem::new$(playlist, provider->createPlaylistItemData(item));
				});
				size_t startOffset = indexMarker->index - lowerBound;
				size_t endOffset = startOffset + tracks.size();
				auto applyItems = [&]() {
					// apply the loaded
					mutator->lock([&]() {
						size_t endInvalidate = list->capacity();
						if(endInvalidate < upperBound) {
							endInvalidate = upperBound;
						}
						if(endInvalidate < page.total) {
							endInvalidate = page.total;
						}
						mutator->invalidate(indexMarker->index, (endInvalidate - indexMarker->index));
						mutator->applyAndResize(lowerBound, page.total, items);
					});
				};
				if(page.items.size() < endOffset) {
					// we don't have enough items, so just apply the items and move on
					applyItems();
					return;
				}
				auto trackIt = tracks.begin();
				for(size_t i=startOffset; i<endOffset; i++) {
					auto& item = items[i];
					auto& track = *trackIt;
					if(item->track()->uri() != track->uri()) {
						// tracks don't match what we expected to be inserted, so just apply items and move on
						applyItems();
						return;
					}
					trackIt++;
				}
				// actually apply items
				auto startIt = std::next(items.begin(), startOffset);
				auto endIt = std::next(startIt, tracks.size());
				auto insertItems = LinkedList<$<PlaylistItem>>(startIt, endIt);
				auto applyingItems = LinkedList<$<PlaylistItem>>(items.begin(), startIt);
				applyingItems.insert(applyingItems.end(), endIt, items.end());
				mutator->lock([&]() {
					mutator->apply(lowerBound, applyingItems);
					mutator->insert(indexMarker->index, insertItems);
					mutator->applyAndResize(lowerBound, page.total, items);
				});
			}, [=](std::exception_ptr error) {
				size_t endInvalidate = list->capacity();
				if(endInvalidate < upperBound) {
					endInvalidate = upperBound;
				}
				mutator->invalidate(indexMarker->index, (endInvalidate - indexMarker->index));
			});
		});
	}



	Promise<void> SpotifyPlaylistMutatorDelegate::appendItems(Mutator* mutator, LinkedList<$<Track>> tracks) {
		size_t chunkSize = getChunkSize();
		size_t halfChunkSize = chunkSize / 2;
		
		auto playlist = this->playlist.lock();
		auto provider = (SpotifyProvider*)playlist->mediaProvider();
		auto uriParts = provider->parseURI(playlist->uri());
		auto list = mutator->getList();
		
		size_t origListSize = list->size().valueOr(0);
		size_t chunkStart = origListSize;
		if(chunkStart > halfChunkSize) {
			chunkStart -= halfChunkSize;
		} else {
			chunkStart = 0;
		}
		
		return loadAPIItems(mutator, chunkStart, chunkSize)
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
			return provider->spotify->addPlaylistTracks(uriParts.id, tracks.map<String>([](auto& track) {
				return track->uri();
			})).then([=](SpotifyPlaylist::AddResult addResult) {
				// update versionId
				auto data = playlist->toData({
					.tracksOffset = 0,
					.tracksLimit = 0
				});
				data.versionId = addResult.snapshotId;
				playlist->applyData(data);
			});
		})
		.then([=]() {
			size_t listSize = list->size().valueOr(0);
			size_t chunkStart = listSize;
			if(chunkStart > halfChunkSize) {
				chunkStart -= halfChunkSize;
			} else {
				chunkStart = 0;
			}
			return loadAPIItems(mutator, chunkStart, chunkSize)
			.except([=](std::exception_ptr error) {
				mutator->lock([&]() {
					size_t newListSize = list->size().valueOr(0);
					if(newListSize == origListSize) {
						mutator->resize(origListSize+1);
					}
					size_t invalidateStart = origListSize;
					if(listSize < invalidateStart) {
						invalidateStart = listSize;
					}
					mutator->invalidate(invalidateStart, 2);
				});
			});
		});
	}



	Promise<void> SpotifyPlaylistMutatorDelegate::removeItems(Mutator* mutator, size_t index, size_t count) {
		size_t chunkSize = getChunkSize();
		size_t halfChunkSize = chunkSize / 2;
		size_t padding = halfChunkSize / 2;
		
		auto playlist = this->playlist.lock();
		auto provider = (SpotifyProvider*)playlist->mediaProvider();
		auto uriParts = provider->parseURI(playlist->uri());
		auto list = mutator->getList();
		
		auto removalIndexes = fgl::new$<LinkedList<AsyncListIndexMarker>>();
		for(size_t i=0; i<count; i++) {
			removalIndexes->pushBack(list->watchIndex(index+i));
		}
		
		size_t lowerBound = index;
		if(lowerBound > padding) {
			lowerBound -= padding;
		} else {
			lowerBound = 0;
		}
		size_t upperBound = index + count + padding;
		
		return loadAPIItemsFromChunks(mutator, lowerBound, (upperBound - lowerBound))
		.then([=]() {
			auto playlistTrackMarkers = ArrayList<Spotify::PlaylistTrackMarker>();
			playlistTrackMarkers.reserve(removalIndexes->size());
			for(auto& indexMarker : *removalIndexes) {
				if(indexMarker->state == AsyncListIndexMarkerState::REMOVED) {
					throw std::runtime_error("item at index "+std::to_string(indexMarker->index)+" was already removed");
				}
				auto item = list->itemAt(indexMarker->index).valueOr(nullptr);
				if(!item) {
					throw std::runtime_error("could not find a valid item at requested removal index "+std::to_string(indexMarker->index));
				}
				bool foundMatch = false;
				for(auto& trackMarker : playlistTrackMarkers) {
					if(trackMarker.uri == item->track()->uri()) {
						foundMatch = true;
						trackMarker.positions.pushBack(indexMarker->index);
						break;
					}
				}
				if(!foundMatch) {
					playlistTrackMarkers.pushBack(Spotify::PlaylistTrackMarker{
						.uri = item->track()->uri(),
						.positions = { indexMarker->index }
					});
				}
			}
			if(playlistTrackMarkers.size() == 0) {
				return Promise<void>::resolve();
			}
			return provider->spotify->removePlaylistTracks(uriParts.id, playlistTrackMarkers)
			.then([=](SpotifyPlaylist::RemoveResult removeResult) {
				// update versionId
				auto data = playlist->toData({
					.tracksOffset = 0,
					.tracksLimit = 0
				});
				data.versionId = removeResult.snapshotId;
				playlist->applyData(data);
			});
		})
		.then([=]() {
			removalIndexes->sort([](auto& a, auto& b) {
				return (a->index >= b->index);
			});
			// process removals
			mutator->lock([&]() {
				Optional<size_t> endRemoveIndex;
				Optional<size_t> startRemoveIndex;
				for(auto& indexMarker : *removalIndexes) {
					if(indexMarker->state == AsyncListIndexMarkerState::REMOVED) {
						size_t index = indexMarker->index;
						size_t count = 2;
						if(index >= 1) {
							index--;
							count++;
						}
						mutator->invalidate(index, count);
					} else {
						if(!endRemoveIndex) {
							endRemoveIndex = indexMarker->index + 1;
						}
						if(!startRemoveIndex || (indexMarker->index + 1) == startRemoveIndex.value()) {
							startRemoveIndex = indexMarker->index;
						} else {
							// we reached the end of a removal chunk
							FGL_ASSERT(startRemoveIndex.value() < endRemoveIndex.value(), "startRemoveIndex should be less than endRemoveIndex");
							mutator->remove(startRemoveIndex.value(), (endRemoveIndex.value() - startRemoveIndex.value()));
							endRemoveIndex = indexMarker->index + 1;
							startRemoveIndex = indexMarker->index;
						}
					}
				}
				if(startRemoveIndex) {
					FGL_ASSERT(startRemoveIndex.value() < endRemoveIndex.value(), "startRemoveIndex should be less than endRemoveIndex");
					mutator->remove(startRemoveIndex.value(), (endRemoveIndex.value() - startRemoveIndex.value()));
				}
			});
		})
		.finally([=]() {
			for(auto& indexMarker : *removalIndexes) {
				list->unwatchIndex(indexMarker);
			}
		});
	}



	Promise<void> SpotifyPlaylistMutatorDelegate::moveItems(Mutator* mutator, size_t index, size_t count, size_t newIndex) {
		if(count == 0 || index == newIndex) {
			return Promise<void>::resolve();
		}
		size_t chunkSize = getChunkSize();
		size_t halfChunkSize = chunkSize / 2;
		size_t padding = halfChunkSize / 2;
		
		auto playlist = this->playlist.lock();
		auto provider = (SpotifyProvider*)playlist->mediaProvider();
		auto uriParts = provider->parseURI(playlist->uri());
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
		
		return loadAPIItemsFromChunks(mutator, lowerBound, (upperBound - lowerBound))
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
			return provider->spotify->movePlaylistTracks(uriParts.id, firstIndexMarker->index, destEndIndexMarker->index, {
				.count=count
			})
			.then([=](SpotifyPlaylist::MoveResult moveResult) {
				// update versionId
				auto data = playlist->toData({
					.tracksOffset = 0,
					.tracksLimit = 0
				});
				data.versionId = moveResult.snapshotId;
				playlist->applyData(data);
			});
		})
		.then([=]() {
			moveIndexes->sort([](auto& a, auto& b) {
				return (a->index <= b->index);
			});
			
			mutator->lock([&]() {
				// check if indexes have changed
				bool indexesChanged = false;
				Optional<size_t> prevIndex;
				for(auto& indexMarker : *moveIndexes) {
					if(indexMarker->state == AsyncListIndexMarkerState::REMOVED) {
						indexesChanged = true;
						break;
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
				if(destIndexMarker->index <= firstIndexMarker->index) {
					if(destIndex >= count) {
						destIndex -= count;
					} else {
						destIndex = 0;
					}
				}
				size_t firstIndex = firstIndexMarker->index;
				auto lastIndexMarker = moveIndexes->back();
				size_t lastIndex = lastIndexMarker->index;
				mutator->move(firstIndex, count, destIndex);
				
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

//
//  YoutubePlaylistMutatorDelegate.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/17/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "YoutubePlaylistMutatorDelegate.hpp"
#include <soundhole/providers/youtube/YoutubeProvider.hpp>
#include <soundhole/database/MediaDatabase.hpp>
#include <soundhole/utils/SoundHoleError.hpp>
#include <tuple>

namespace sh {
	YoutubePlaylistMutatorDelegate::YoutubePlaylistMutatorDelegate($<Playlist> playlist)
	: playlist(playlist), pageSize(50) {
		//
	}

	size_t YoutubePlaylistMutatorDelegate::getChunkSize() const {
		return pageSize;
	}

	ArrayList<YoutubePlaylistMutatorDelegate::Range> YoutubePlaylistMutatorDelegate::coverRange(Range range, Range coverRange) const {
		if(range.count == 0) {
			return {};
		}
		size_t upperRange = range.index + range.count;
		size_t upperCoverRange = coverRange.index + coverRange.count;
		if(range.index < coverRange.index) {
			if(upperRange <= coverRange.index) {
				return {
					range
				};
			} else if(upperRange <= upperCoverRange) {
				return {
					Range{
						.index=range.index,
						.count=(coverRange.index-range.index)
					}
				};
			} else {
				return {
					Range{
						.index=upperCoverRange,
						.count=(upperRange-upperCoverRange)
					},
					Range{
						.index=range.index,
						.count=(coverRange.index-range.index)
					}
				};
			}
		} else if(range.index < upperCoverRange) {
			if(upperRange <= upperCoverRange) {
				return {};
			} else {
				return {
					Range{
						.index=upperCoverRange,
						.count=(upperRange - upperCoverRange)
					}
				};
			}
		} else {
			return {
				range
			};
		}
	}

	ArrayList<YoutubePlaylistMutatorDelegate::Dist> YoutubePlaylistMutatorDelegate::coverRangeDist(Range range, Range coverRange) const {
		size_t upperRange = range.index + range.count;
		size_t upperCoverRange = coverRange.index + coverRange.count;
		if(coverRange.index <= range.index) {
			if(upperRange <= upperCoverRange) {
				return {
					Dist{
						.count=(upperRange - range.index),
						.reverse=false
					}
				};
			} else {
				return {
					Dist{
						.count=(upperRange - coverRange.index),
						.reverse=false
					}
				};
			}
		} else if(upperCoverRange < upperRange) {
			return {
				Dist{
					.count=(upperCoverRange - range.index),
					.reverse=true
				},
				Dist{
					.count=(upperRange - coverRange.index),
					.reverse=false
				}
			};
		} else {
			return {
				Dist{
					.count=(upperCoverRange - range.index),
					.reverse=true
				}
			};
		}
	}

	Promise<void> YoutubePlaylistMutatorDelegate::loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) {
		if(count == 0) {
			return Promise<void>::resolve();
		}
		auto playlist = this->playlist.lock();
		if(options.offline && options.database != nullptr) {
			// offline load
			return options.database->loadPlaylistItems(playlist, mutator, index, count);
		}
		else {
			// online load
			auto range = Range{.index=index,.count=count};
			auto pageTokens = this->pageTokens + LinkedList<PageToken>{ PageToken{"", 0} };
			Optional<std::pair<PageToken,ArrayList<Dist>>> closestPageToken;
			size_t closestDistTotal = -1;
			for(auto& pageToken : pageTokens) {
				auto tokenRange = Range{
					.index=pageToken.offset,
					.count=pageSize
				};
				auto dists = coverRangeDist(range, tokenRange);
				size_t distTotal = 0;
				for(auto& dist : dists) {
					distTotal += dist.count;
				}
				if(!closestPageToken || distTotal < closestDistTotal) {
					closestPageToken = { pageToken, dists };
					closestDistTotal = distTotal;
				}
			}
			FGL_ASSERT(closestPageToken->second.size() > 0, "for some reason we have 0 dists");
			closestPageToken->second.sort([](auto& a, auto& b) {
				if(a.reverse == b.reverse || (a.reverse && !b.reverse)) {
					return true;
				}
				return false;
			});
			auto promise = Promise<void>::resolve();
			for(auto& dist : closestPageToken->second) {
				promise = promise.then([=]() {
					if(dist.reverse) {
						return loadItemsToIndex(mutator, index, closestPageToken->first.value, closestPageToken->first.offset, true);
					} else {
						return loadItemsToIndex(mutator, (index+count-1), closestPageToken->first.value, closestPageToken->first.offset, false);
					}
				});
			}
			return promise;
		}
	}

	Promise<YoutubePlaylistMutatorDelegate::LoadPager> YoutubePlaylistMutatorDelegate::loadItems(Mutator* mutator, String pageToken) {
		auto playlist = this->playlist.lock();
		auto provider = (YoutubeProvider*)playlist->mediaProvider();
		auto playlistId = provider->parseURI(playlist->uri()).id;
		return provider->youtube->getPlaylistItems(playlistId, {
			.maxResults=pageSize,
			.pageToken=pageToken
		}).then([=](YoutubePage<YoutubePlaylistItem> page) {
			mutator->lock([&]() {
				size_t insertStartIndex = -1;
				LinkedList<$<PlaylistItem>> applyItems;
				size_t prevIndex = -1;
				for(auto& item : page.items) {
					auto playlistItem = PlaylistItem::new$(playlist, provider->createPlaylistItemData(item));
					if(applyItems.size() == 0) {
						applyItems.pushBack(playlistItem);
						insertStartIndex = item.snippet.position;
						prevIndex = item.snippet.position;
					} else if(item.snippet.position == (prevIndex+1)) {
						applyItems.pushBack(playlistItem);
						prevIndex = item.snippet.position;
					} else {
						mutator->applyAndResize(insertStartIndex, page.pageInfo.totalResults, applyItems);
						applyItems.clear();
						applyItems.pushBack(playlistItem);
						insertStartIndex = item.snippet.position;
					}
				}
				if(applyItems.size() > 0) {
					mutator->applyAndResize(insertStartIndex, page.pageInfo.totalResults, applyItems);
				}
			});
			return Promise<LoadPager>::resolve(LoadPager{
				.prevPageToken=page.prevPageToken,
				.nextPageToken=page.nextPageToken
			});
		});
	}

	Promise<void> YoutubePlaylistMutatorDelegate::loadItemsToIndex(Mutator* mutator, size_t targetIndex, String pageToken, size_t currentIndex, bool reverse) {
		return loadItems(mutator, pageToken).then([=](LoadPager result) {
			size_t prevPageIndex = (currentIndex >= pageSize) ? (currentIndex-pageSize) : 0;
			size_t nextPageIndex = (currentIndex+pageSize);
			if(!result.prevPageToken.empty()) {
				pageTokens.pushBack(PageToken{
					.value=result.prevPageToken,
					.offset=prevPageIndex
				});
			}
			if(!result.nextPageToken.empty()) {
				pageTokens.pushBack(PageToken{
					.value=result.nextPageToken,
					.offset=nextPageIndex
				});
			}
			if(reverse) {
				if(currentIndex <= targetIndex) {
					return Promise<void>::resolve();
				}
			} else {
				if(nextPageIndex > targetIndex) {
					return Promise<void>::resolve();
				}
			}
			size_t nextIndex = reverse ? prevPageIndex : nextPageIndex;
			String nextPageToken = reverse ? result.prevPageToken : result.nextPageToken;
			return loadItemsToIndex(mutator, targetIndex, nextPageToken, nextIndex, reverse);
		});
	}



	Promise<void> YoutubePlaylistMutatorDelegate::insertItems(Mutator* mutator, size_t index, LinkedList<$<Track>> tracks) {
		auto playlist = this->playlist.lock();
		auto provider = (YoutubeProvider*)playlist->mediaProvider();
		auto playlistId = provider->parseURI(playlist->uri()).id;
		
		auto promise = Promise<void>::resolve();
		size_t i = index;
		auto trackIds = tracks.map<String>([=](auto& track) {
			return provider->parseURI(track->uri()).id;
		});
		for(auto trackId : trackIds) {
			promise = promise.then([=]() {
				return provider->youtube->insertPlaylistItem(playlistId, trackId, {
					.position = i
				}).then([=](YoutubePlaylistItem youtubeItem) {
					auto item = PlaylistItem::new$(playlist, provider->createPlaylistItemData(youtubeItem));
					mutator->lock([&]() {
						mutator->insert(youtubeItem.snippet.position, { item });
					});
				});
			});
		}
		return promise;
	}

	Promise<void> YoutubePlaylistMutatorDelegate::appendItems(Mutator* mutator, LinkedList<$<Track>> tracks) {
		auto playlist = this->playlist.lock();
		auto provider = (YoutubeProvider*)playlist->mediaProvider();
		auto playlistId = provider->parseURI(playlist->uri()).id;
		
		auto trackIds = tracks.map<String>([=](auto& track) {
			return provider->parseURI(track->uri()).id;
		});
		auto promise = Promise<void>::resolve();
		for(auto trackId : trackIds) {
			promise = promise.then([=]() {
				return provider->youtube->insertPlaylistItem(playlistId, trackId).then([=](YoutubePlaylistItem youtubeItem) {
					auto item = PlaylistItem::new$(playlist, provider->createPlaylistItemData(youtubeItem));
					mutator->lock([&]() {
						mutator->insert(youtubeItem.snippet.position, { item });
					});
				});
			});
		}
		return promise;
	}

	Promise<void> YoutubePlaylistMutatorDelegate::removeItems(Mutator* mutator, size_t index, size_t count) {
		auto playlist = this->playlist.lock();
		auto provider = (YoutubeProvider*)playlist->mediaProvider();
		auto playlistId = provider->parseURI(playlist->uri()).id;
		
		auto list = mutator->getList();
		auto items = list->getLoadedItems({
			.startIndex = index,
			.limit = count,
			.onlyValidItems = false
		});
		size_t itemIdOffset = 0;
		auto itemMarkers = items.map<std::tuple<String,AsyncListIndexMarker>>([&](auto& item) {
			auto itemId = item->uniqueId();
			if(itemId.empty()) {
				throw std::runtime_error("Missing youtubePlaylistItemId prop for item at index "+std::to_string(index+itemIdOffset));
			}
			itemIdOffset++;
			auto indexMarker = list->watchIndex(index+itemIdOffset);
			return std::tuple<String,AsyncListIndexMarker>(itemId, indexMarker);
		});
		
		auto promise = Promise<void>::resolve();
		auto removedItemIds = LinkedList<String>();
		for(auto& [ itemId, indexMarker ] : itemMarkers) {
			if(!removedItemIds.contains(itemId)) {
				auto uniqueItemId = itemId;
				promise = promise.then([=]() {
					return provider->youtube->deletePlaylistItem(uniqueItemId).then([=]() {
						auto indexMarkers = itemMarkers
							.where([&](auto& tuple) { return std::get<0>(tuple) == uniqueItemId; })
							.map<AsyncListIndexMarker>([&](auto& tuple) { return std::get<1>(tuple); });
						indexMarkers.sort([](auto& a, auto& b) {
							return a->index >= b->index;
						});
						mutator->lock([&]() {
							for(auto& indexMarker : indexMarkers) {
								if(indexMarker->state == AsyncListIndexMarkerState::REMOVED) {
									size_t index = indexMarker->index;
									size_t count = 2;
									if(index >= 1) {
										index--;
										count++;
									}
									mutator->invalidate(index, count);
								} else {
									mutator->remove(indexMarker->index, 1);
								}
							}
						});
					});
				});
			}
			removedItemIds.pushBack(itemId);
		}
		
		promise = promise.finally([=]() {
			for(auto& [ itemId, indexMarker ] : itemMarkers) {
				list->unwatchIndex(indexMarker);
			}
		});
		return promise;
	}

	Promise<void> YoutubePlaylistMutatorDelegate::moveItems(Mutator* mutator, size_t index, size_t count, size_t newIndex) {
		return Promise<void>::reject(std::logic_error("Not yet implemented"));
	}
}

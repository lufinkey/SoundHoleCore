//
//  PlaybackQueue.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/23/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "PlaybackQueue.hpp"

namespace sh {
	bool PlaybackQueue::shiftToItem($<QueueItem> item) {
		auto it = items.findEqual(item);
		if(it != items.end()) {
			auto itemsEndIt = std::next(it, 1);
			// extract items from items and move to the end of pastItems
			pastItems.splice(pastItems.end(), items, items.begin(), itemsEndIt);
			return true;
		}
		it = pastItems.findEqual(item);
		if(it != pastItems.end()) {
			if(it == std::prev(pastItems.end(), 1)) {
				// given item is already current item
				return false;
			}
			auto pastItemsStartIt = std::next(it, 1);
			// the given item becomes last item of pastItems, elements after it move to beginning of items
			items.splice(items.begin(), pastItems, pastItemsStartIt, pastItems.end());
			return true;
		}
		// item doesn't exist, so just add to pastItems
		pastItems.pushBack(item);
		return true;
	}

	bool PlaybackQueue::shiftBehindItem($<QueueItem> item) {
		auto it = pastItems.findEqual(item);
		if(it == pastItems.end()) {
			return false;
		}
		items.splice(items.begin(), pastItems, it, pastItems.end());
		return true;
	}

	bool PlaybackQueue::removeItem($<QueueItem> item) {
		if(items.removeFirstEqual(item)) {
			return true;
		}
		if(pastItems.removeFirstEqual(item)) {
			return true;
		}
		return false;
	}

	void PlaybackQueue::clear() {
		items.clear();
		pastItems.clear();
	}

	void PlaybackQueue::clearPastItems() {
		pastItems.clear();
	}

	$<QueueItem> PlaybackQueue::prependItem($<Track> track) {
		auto queueItem = QueueItem::new$(track);
		items.pushFront(queueItem);
		return queueItem;
	}

	$<QueueItem> PlaybackQueue::appendItem($<Track> track) {
		auto queueItem = QueueItem::new$(track);
		items.pushBack(queueItem);
		return queueItem;
	}

	$<QueueItem> PlaybackQueue::insertItemRandomly($<Track> track) {
		auto queueItem = QueueItem::new$(track);
		double randVal = ((double)std::rand() / (double)RAND_MAX);
		size_t index = (size_t)(randVal * (double)(items.size() + 1));
		if(index > items.size()) {
			index = items.size();
		}
		auto it = std::next(items.begin(), index);
		items.insert(it, queueItem);
		return queueItem;
	}
}

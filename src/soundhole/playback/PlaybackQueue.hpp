//
//  PlaybackQueue.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/23/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/Track.hpp>
#include <soundhole/media/QueueItem.hpp>

namespace sh {
	class PlaybackQueue {
	public:
		bool shiftToItem($<QueueItem> queueItem);
		bool shiftBehindItem($<QueueItem> queueItem);
		bool removeItem($<QueueItem> queueItem);
		void clear();
		void clearPastItems();
		bool isPastItem($<QueueItem> queueItem);
		
		$<QueueItem> prependItem($<Track> track);
		$<QueueItem> appendItem($<Track> track);
		$<QueueItem> insertItemRandomly($<Track> track);
		
		LinkedList<$<QueueItem>> pastItems;
		LinkedList<$<QueueItem>> items;
	};
}

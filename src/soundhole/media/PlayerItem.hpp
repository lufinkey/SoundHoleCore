//
//  PlayerItem.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/1/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "Track.hpp"
#include "QueueItem.hpp"
#include "TrackCollection.hpp"

namespace sh {
	class PlayerItem: public Variant<$<Track>,$<QueueItem>,$<TrackCollectionItem>> {
	public:
		using Variant::Variant;
		using Variant::get;
		using Variant::getOr;
		using Variant::is;
		
		$<Track> track() const;
		
		bool isTrack() const;
		$<Track> asTrack() const;
		bool isCollectionItem() const;
		$<TrackCollectionItem> asCollectionItem() const;
		bool isQueueItem() const;
		$<QueueItem> asQueueItem() const;
		
		Json toJson() const;
		
		bool matchesItem(const PlayerItem& item) const;
	};
}

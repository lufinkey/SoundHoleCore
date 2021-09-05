//
//  PlayerItem.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/1/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "PlayerItem.hpp"

namespace sh {
	$<Track> PlayerItem::track() const {
		if(isTrack()) {
			return asTrack();
		} else if(isCollectionItem()) {
			return asCollectionItem()->track();
		} else if(isQueueItem()) {
			return asQueueItem()->track();
		}
		throw std::logic_error("invalid state for PlayerItem");
	}

	bool PlayerItem::isTrack() const {
		return is<$<Track>>();
	}

	$<Track> PlayerItem::asTrack() const {
		return getOr<$<Track>>(nullptr);
	}

	bool PlayerItem::isCollectionItem() const {
		return is<$<TrackCollectionItem>>();
	}

	$<TrackCollectionItem> PlayerItem::asCollectionItem() const {
		return getOr<$<TrackCollectionItem>>(nullptr);
	}

	bool PlayerItem::isQueueItem() const {
		return is<$<QueueItem>>();
	}

	$<QueueItem> PlayerItem::asQueueItem() const {
		return getOr<$<QueueItem>>(nullptr);
	}

	Json PlayerItem::toJson() const {
		if(isTrack()) {
			return asTrack()->toJson();
		} else if(isCollectionItem()) {
			return asCollectionItem()->toJson();
		} else if(isQueueItem()) {
			return asQueueItem()->toJson();
		}
		throw std::logic_error("invalid state for PlayerItem");
	}

	bool PlayerItem::matchesItem(const PlayerItem& cmp) const {
		if(type() != cmp.type()) {
			return false;
		}
		if(isTrack()) {
			return asTrack()->uri() == cmp.asTrack()->uri();
		} else if(isCollectionItem()) {
			return asCollectionItem()->matchesItem(cmp.asCollectionItem().get());
		} else if(isQueueItem()) {
			return asQueueItem()->track()->uri() == cmp.asQueueItem()->track()->uri();
		}
		throw std::logic_error("invalid state for PlayerItem");
	}
}

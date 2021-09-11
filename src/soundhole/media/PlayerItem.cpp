//
//  PlayerItem.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/1/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "PlayerItem.hpp"
#include "OmniTrack.hpp"
#include "MediaProvider.hpp"

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

	bool PlayerItem::matches(const PlayerItem& cmp) const {
		if(type() != cmp.type()) {
			return false;
		}
		if(isTrack()) {
			return asTrack()->matches(cmp.asTrack().get());
		} else if(isCollectionItem()) {
			auto collectionItem = asCollectionItem();
			auto cmpCollectionItem = cmp.asCollectionItem();
			if(!collectionItem && !cmpCollectionItem) {
				return true;
			} else if(!collectionItem || !cmpCollectionItem) {
				return false;
			}
			auto context = collectionItem ? collectionItem->context().lock() : nullptr;
			auto cmpContext = cmpCollectionItem ? cmpCollectionItem->context().lock() : nullptr;
			if(context && cmpContext && (context->uri() != cmpContext->uri() || context->mediaProvider()->name() != cmpContext->mediaProvider()->name())) {
				return false;
			}
			return collectionItem->matchesItem(cmpCollectionItem.get());
		} else if(isQueueItem()) {
			return asQueueItem()->matches(cmp.asQueueItem().get());
		}
		throw std::logic_error("invalid state for PlayerItem");
	}

	bool PlayerItem::linksTrack($<const Track> track) const {
		return linkedTrackWhere([&](auto& cmpTrack) {
			return track->matches(cmpTrack.get());
		}) != nullptr;
	}
}

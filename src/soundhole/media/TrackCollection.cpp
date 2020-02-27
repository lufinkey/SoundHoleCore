//
//  TrackCollection.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "TrackCollection.hpp"

namespace sh {
	TrackCollectionItem::TrackCollectionItem($<TrackCollectionItem>& ptr, $<TrackCollection> context, Data data)
	: _context(context), _track(data.track) {
		ptr = $<TrackCollectionItem>(this);
		weakSelf = ptr;
	}

	$<TrackCollectionItem> TrackCollectionItem::self() {
		return weakSelf.lock();
	}

	$<const TrackCollectionItem> TrackCollectionItem::self() const {
		return std::static_pointer_cast<const TrackCollectionItem>(weakSelf.lock());
	}

	$<Track> TrackCollectionItem::track() {
		return _track;
	}

	$<const Track> TrackCollectionItem::track() const {
		return std::static_pointer_cast<const Track>(_track);
	}

	w$<TrackCollection> TrackCollectionItem::context() {
		return _context;
	}

	w$<const TrackCollection> TrackCollectionItem::context() const {
		return _context;
	}
	
	Optional<size_t> TrackCollectionItem::indexInContext() const {
		auto context = _context.lock();
		if(!context) {
			// TODO maybe fatal error?
			return std::nullopt;
		}
		return context->indexOfItem(this);
	}

	TrackCollectionItem::Data TrackCollectionItem::toData() const {
		return TrackCollectionItem::Data{
			.track=_track
		};
	}

	Json TrackCollectionItem::toJson() const {
		return Json::object{
			{ "type", "collectionItem" },
			{ "track", _track->toJson() }
		};
	}

	TrackCollectionItem::TrackCollectionItem($<TrackCollectionItem>& ptr, $<TrackCollection> context, Json json, FromJsonOptions options)
	: _context(context), _track(Track::fromJson(json, options)) {
		ptr = $<TrackCollectionItem>(this);
		weakSelf = ptr;
	}

	Json TrackCollection::toJson() const {
		return toJson(ToJsonOptions());
	}

	Json TrackCollection::toJson(ToJsonOptions options) const {
		auto json = MediaItem::toJson().object_items();
		auto itemCount = this->itemCount();
		json.merge(Json::object{
			{ "itemCount", itemCount ? Json((double)itemCount.value()) : Json() }
		});
		return json;
	}
}

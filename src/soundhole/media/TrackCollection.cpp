//
//  TrackCollection.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "TrackCollection.hpp"
#include <soundhole/database/MediaDatabase.hpp>

namespace sh {
	TrackCollectionItem::TrackCollectionItem($<TrackCollection> context, Data data)
	: _context(context), _track(data.track) {
		//
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

	TrackCollectionItem::TrackCollectionItem($<TrackCollection> context, Json json, MediaProviderStash* stash)
	: _context(context), _track(Track::fromJson(json, stash)) {
		//
	}



	std::map<String,Any> TrackCollection::LoadItemOptions::toDict() const {
		return {
			{ "database", database }
		};
	}

	TrackCollection::LoadItemOptions TrackCollection::LoadItemOptions::fromDict(std::map<String,Any> dict) {
		return {
			.database = dict["database"].maybeAs<MediaDatabase*>().valueOr(nullptr)
		};
	}

	TrackCollection::LoadItemOptions TrackCollection::LoadItemOptions::defaultOptions() {
		return LoadItemOptions();
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

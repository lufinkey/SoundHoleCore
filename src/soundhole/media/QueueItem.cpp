//
//  QueueItem.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/30/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "QueueItem.hpp"
#include "MediaProvider.hpp"

namespace sh {
	$<QueueItem> QueueItem::new$($<Track> track) {
		return fgl::new$<QueueItem>(QueueItem::Data{
			.track = track,
			.addedAt = Date::now()
		});
	}

	$<QueueItem> QueueItem::new$(const Data& data) {
		return fgl::new$<QueueItem>(data);
	}

	QueueItem::QueueItem(const Data& data)
	: _track(data.track), _addedAt(data.addedAt) {
		//
	}

	QueueItem::~QueueItem() {
		//
	}

	bool QueueItem::matches(const QueueItem* cmp) const {
		return _track->matches(cmp->_track.get()) && _addedAt == cmp->_addedAt;
	}

	$<Track> QueueItem::track() {
		return _track;
	}

	$<const Track> QueueItem::track() const {
		return std::static_pointer_cast<const Track>(_track);
	}

	const Date& QueueItem::addedAt() const {
		return _addedAt;
	}


	$<QueueItem> QueueItem::fromJson(const Json& json, MediaProviderStash* stash) {
		auto mediaItem = stash->parseMediaItem(json["track"]);
		if(!mediaItem) {
			throw std::invalid_argument("Invalid json for QueueItem: 'track' cannot be null");
		}
		auto track = std::dynamic_pointer_cast<Track>(mediaItem);
		if(!track) {
			throw std::invalid_argument("Invalid json for QueueItem: 'track' property cannot have type '"+mediaItem->type()+"'");
		}
		auto addedAtJson = json["addedAt"];
		return fgl::new$<QueueItem>(QueueItem::Data{
			.track = track,
			.addedAt = Date::maybeFromISOString(addedAtJson.string_value())
				.valueOrThrow(std::invalid_argument("Invalid json for QueueItem: "+addedAtJson.dump()+" is not a valid date for 'addedAt'"))
		});
	}

	Json QueueItem::toJson() const {
		return Json::object{
			{ "type", "queueItem" },
			{ "track", _track->toJson() },
			{ "addedAt", (std::string)_addedAt.toISOString() }
		};
	}
}

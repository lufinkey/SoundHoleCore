//
//  QueueItem.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/30/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "QueueItem.hpp"

namespace sh {
	$<QueueItem> QueueItem::new$($<Track> track) {
		return fgl::new$<QueueItem>(track);
	}

	QueueItem::QueueItem($<Track> track)
	: _track(track) {
		//
	}

	$<Track> QueueItem::track() {
		return _track;
	}

	$<const Track> QueueItem::track() const {
		return std::static_pointer_cast<const Track>(_track);
	}


	$<QueueItem> QueueItem::fromJson(Json json, const FromJsonOptions& options) {
		return fgl::new$<QueueItem>(json, options);
	}

	QueueItem::QueueItem(Json json, const FromJsonOptions& options)
	: _track(Track::fromJson(json["track"], options)) {
		//
	}

	Json QueueItem::toJson() const {
		return Json::object{
			{ "type", "queueItem" },
			{ "track", _track->toJson() }
		};
	}
}

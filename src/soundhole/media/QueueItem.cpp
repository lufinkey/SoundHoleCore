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
		return fgl::new$<QueueItem>(track);
	}

	QueueItem::QueueItem($<Track> track)
	: _track(track) {
		//
	}

	QueueItem::~QueueItem() {
		//
	}

	$<Track> QueueItem::track() {
		return _track;
	}

	$<const Track> QueueItem::track() const {
		return std::static_pointer_cast<const Track>(_track);
	}


	$<QueueItem> QueueItem::fromJson(const Json& json, MediaProviderStash* stash) {
		auto trackJson = json["track"];
		auto providerName = trackJson["provider"];
		if(!providerName.is_string()) {
			throw std::invalid_argument("artist provider must be a string");
		}
		auto provider = stash->getMediaProvider(providerName.string_value());
		if(provider == nullptr) {
			throw std::invalid_argument("invalid provider name for track: "+providerName.string_value());
		}
		return fgl::new$<QueueItem>(provider->track(Track::Data::fromJson(trackJson, stash)));
	}

	Json QueueItem::toJson() const {
		return Json::object{
			{ "type", "queueItem" },
			{ "track", _track->toJson() }
		};
	}
}

//
//  UnmatchedScrobble.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/16/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#include "UnmatchedScrobble.hpp"
#include "Scrobble.hpp"
#include "Artist.hpp"

namespace sh {
	bool UnmatchedScrobble::equals(const UnmatchedScrobble& cmp) const {
		return scrobbler == cmp.scrobbler
			&& (historyItem == cmp.historyItem
				|| (historyItem->startTime() == cmp.historyItem->startTime()
					&& historyItem->track()->uri() == cmp.historyItem->track()->uri()));
	}

	bool UnmatchedScrobble::matchesInexactly($<Scrobble> scrobble) const {
		auto startTimeStart = this->historyItem->startTime() - std::chrono::minutes(1);
		auto startTimeEnd = this->historyItem->startTime() + std::chrono::minutes(1);
		auto track = this->historyItem->track();
		auto artist = track->artists().empty() ? nullptr : track->artists().front();
		return scrobbler == scrobble->scrobbler()
			&& startTimeStart <= scrobble->startTime()
			&& scrobble->startTime() <= startTimeEnd
			&& scrobble->trackName().startsWith(track->name())
			&& (artist == nullptr || scrobble->artistName().startsWith(artist->name()));
	}

	UnmatchedScrobble UnmatchedScrobble::fromJson(const Json& json, MediaProviderStash* mediaProviderStash, ScrobblerStash* scrobblerStash) {
		return UnmatchedScrobble{
			.scrobbler = scrobblerStash->getScrobbler(json["scrobbler"].string_value()),
			.historyItem = PlaybackHistoryItem::fromJson(json["historyItem"], mediaProviderStash)
		};
	}
}

//
//  UnmatchedScrobble.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/16/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#include "UnmatchedScrobble.hpp"

namespace sh {
	UnmatchedScrobble UnmatchedScrobble::fromJson(const Json& json, MediaProviderStash* mediaProviderStash, ScrobblerStash* scrobblerStash) {
		return UnmatchedScrobble{
			.scrobbler = scrobblerStash->getScrobbler(json["scrobbler"].string_value()),
			.historyItem = PlaybackHistoryItem::fromJson(json["historyItem"], mediaProviderStash)
		};
	}
}

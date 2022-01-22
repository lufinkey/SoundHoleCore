//
//  UnmatchedScrobble.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/16/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/MediaProviderStash.hpp>
#include <soundhole/media/ScrobblerStash.hpp>
#include <soundhole/media/PlaybackHistoryItem.hpp>

namespace sh {
	struct UnmatchedScrobble {
		Scrobbler* scrobbler;
		$<PlaybackHistoryItem> historyItem;
		
		bool equals(const UnmatchedScrobble&) const;
		bool matchesInexactly($<Scrobble> scrobble) const;
		
		static UnmatchedScrobble fromJson(const Json& json, MediaProviderStash* mediaProviderStash, ScrobblerStash* scrobblerStash);
	};
}

//
//  QueueItem.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/30/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "Track.hpp"

namespace sh {
	class QueueItem: std::enable_shared_from_this<QueueItem> {
	public:
		static $<QueueItem> new$($<Track> track);
		QueueItem($<Track> track);
		virtual ~QueueItem();
		
		$<Track> track();
		$<const Track> track() const;
		
		static $<QueueItem> fromJson(const Json& json, MediaProviderStash* stash);
		virtual Json toJson() const;
		
	private:
		$<Track> _track;
	};
}

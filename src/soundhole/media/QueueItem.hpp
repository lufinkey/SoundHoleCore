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
		using FromJsonOptions = MediaItem::FromJsonOptions;
		
		static $<QueueItem> new$($<Track> track);
		
		QueueItem($<Track> track);
		QueueItem(Json json, const FromJsonOptions& options);
		
		$<Track> track();
		$<const Track> track() const;
		
		static $<QueueItem> fromJson(Json json, const FromJsonOptions& options);
		virtual Json toJson() const;
		
	private:
		$<Track> _track;
	};
}

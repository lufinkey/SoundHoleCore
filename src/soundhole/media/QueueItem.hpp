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
	class QueueItem {
	public:
		static $<QueueItem> new$($<Track> track);
		
		QueueItem($<Track> track);
		
		$<Track> track();
		$<const Track> track() const;
		
	private:
		$<Track> _track;
	};
}

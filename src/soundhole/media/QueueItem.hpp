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
		struct Data {
			$<Track> track;
			Date addedAt;
		};
		
		static $<QueueItem> new$($<Track> track);
		static $<QueueItem> new$(const Data& data);
		
		QueueItem(const Data& data);
		virtual ~QueueItem();
		
		bool matches(const QueueItem*) const;
		
		$<Track> track();
		$<const Track> track() const;
		
		const Date& addedAt() const;
		
		static $<QueueItem> fromJson(const Json& json, MediaProviderStash* stash);
		virtual Json toJson() const;
		
	private:
		$<Track> _track;
		Date _addedAt;
	};
}

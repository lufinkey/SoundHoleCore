//
//  PlaybackHistoryItem.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/22/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "Track.hpp"
#include "PlayerItem.hpp"

namespace sh {
	class PlaybackHistoryItem: public std::enable_shared_from_this<PlaybackHistoryItem> {
	public:
		static constexpr double FALLBACK_DURATION = 120.0;
		
		struct Data {
			$<Track> track;
			Date startTime;
			String contextURI;
			double duration;
			bool chosenByUser;
			
			static Data fromJson(const Json& json, MediaProviderStash* stash);
		};
		
		static $<PlaybackHistoryItem> new$(const Data& data);
		
		PlaybackHistoryItem(const Data& data);
		virtual ~PlaybackHistoryItem();
		
		$<Track> track();
		$<const Track> track() const;
		
		const Date& startTime() const;
		const String& contextURI() const;
		double duration() const;
		bool chosenByUser() const;
		
		void increaseDuration(double amount);
		void setDuration(double duration);
		
		bool matchesItem(const PlayerItem&) const;
		
		Data toData() const;
		
	private:
		$<Track> _track;
		Date _startTime;
		String _contextURI;
		double _duration;
		bool _chosenByUser;
	};
}

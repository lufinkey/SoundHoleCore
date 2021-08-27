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

namespace sh {
	class PlaybackHistoryItem: public std::enable_shared_from_this<PlaybackHistoryItem> {
	public:
		struct Data {
			$<Track> track;
			Date startTime;
			String contextURI;
			Optional<double> duration;
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
		const Optional<double>& duration() const;
		bool chosenByUser() const;
		
		void increaseDuration(double amount);
		
	private:
		$<Track> _track;
		Date _startTime;
		String _contextURI;
		Optional<double> _duration;
		bool _chosenByUser;
	};
}

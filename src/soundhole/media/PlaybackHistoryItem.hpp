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
		friend class PlayerHistoryManager;
	public:
		enum class Visibility: uint8_t {
			UNSAVED,
			SCROBBLES,
			HISTORY
		};
		static Visibility Visibility_fromString(const String&);
		static String Visibility_toString(Visibility);
		
		static constexpr double FALLBACK_DURATION = 240.0;
		
		struct Data {
			$<Track> track;
			Date startTime;
			String contextURI;
			Optional<double> duration;
			bool chosenByUser;
			Visibility visibility;
			
			static Data fromJson(const Json& json, MediaProviderStash* stash);
		};
		
		static $<PlaybackHistoryItem> new$(Data data);
		static $<PlaybackHistoryItem> fromJson(const Json& json, MediaProviderStash* stash);
		
		PlaybackHistoryItem(Data data);
		virtual ~PlaybackHistoryItem();
		
		$<Track> track();
		$<const Track> track() const;
		
		const Date& startTime() const;
		const String& contextURI() const;
		const Optional<double>& duration() const;
		bool chosenByUser() const;
		Visibility visibility() const;
		
		void increaseDuration(double amount);
		void setDuration(Optional<double> duration);
		
		bool matchesItem(const PlayerItem&) const;
		bool matches(const PlaybackHistoryItem*) const;
		
		Data toData() const;
		Json toJson() const;
		
	private:
		$<Track> _track;
		Date _startTime;
		String _contextURI;
		Optional<double> _duration;
		bool _chosenByUser;
		Visibility _visibility;
	};
}

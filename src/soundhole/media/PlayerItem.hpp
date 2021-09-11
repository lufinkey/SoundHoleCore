//
//  PlayerItem.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/1/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "Track.hpp"
#include "OmniTrack.hpp"
#include "QueueItem.hpp"
#include "TrackCollection.hpp"

namespace sh {
	class PlayerItem: public Variant<$<Track>,$<QueueItem>,$<TrackCollectionItem>> {
	public:
		using Variant::Variant;
		using Variant::get;
		using Variant::getOr;
		using Variant::is;
		
		$<Track> track() const;
		
		bool isTrack() const;
		$<Track> asTrack() const;
		bool isCollectionItem() const;
		$<TrackCollectionItem> asCollectionItem() const;
		bool isQueueItem() const;
		$<QueueItem> asQueueItem() const;
		
		template<typename Predicate>
		$<Track> linkedTrackWhere(Predicate predicate) const;
		
		bool matches(const PlayerItem&) const;
		bool linksTrack($<const Track> track) const;
		
		Json toJson() const;
		
		bool matchesItem(const PlayerItem& item) const;
	};



	#pragma mark PlayerItem implementation

	template<typename Predicate>
	$<Track> PlayerItem::linkedTrackWhere(Predicate predicate) const {
		auto track = this->track();
		if(auto omniTrack = track.as<OmniTrack>()) {
			if(auto linkedTrack = omniTrack->linkedTrackWhere(predicate)) {
				return linkedTrack;
			}
		}
		if(predicate(track)) {
			return track;
		}
		return nullptr;
	}
}

//
//  OmniTrack.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/7/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "Track.hpp"

namespace sh {
	class OmniTrack: public Track {
	public:
		struct Data: public Track::Data {
			String musicBrainzID;
			ArrayList<$<Track>> linkedTracks;
			
			static Data fromJson(const Json& json, MediaProviderStash* stash);
		};
		
		static $<OmniTrack> new$(MediaProvider* provider, const Data& data);
		OmniTrack(MediaProvider* provider, const Data& data);
		
		virtual bool isPlayable() const override;
		
		const String& musicBrainzID() const;
		
		const ArrayList<$<Track>>& linkedTracks();
		const ArrayList<$<const Track>>& linkedTracks() const;
		template<typename Predicate>
		$<Track> linkedTrackWhere(Predicate predicate);
		template<typename Predicate>
		$<const Track> linkedTrackWhere(Predicate predicate) const;
		
		virtual Promise<void> fetchData() override;
		void applyData(const Data& data);
		
		Data toData() const;
		virtual Json toJson() const override;
		
	private:
		String _musicBrainzID;
		ArrayList<$<Track>> _linkedTracks;
	};



	#pragma mark OmniTrack implementation

	template<typename Predicate>
	$<Track> OmniTrack::linkedTrackWhere(Predicate predicate) {
		return _linkedTracks.firstWhere(predicate, nullptr);
	}

	template<typename Predicate>
	$<const Track> OmniTrack::linkedTrackWhere(Predicate predicate) const {
		return _linkedTracks.firstWhere(predicate, nullptr);
	}
}

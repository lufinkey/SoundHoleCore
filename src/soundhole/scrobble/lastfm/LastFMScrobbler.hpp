//
//  LastFMScrobbler.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/6/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/scrobble/Scrobbler.hpp>
#include <soundhole/providers/lastfm/api/LastFM.hpp>

namespace sh {
	class LastFMScrobbler: public Scrobbler {
	public:
		LastFMScrobbler(LastFM*);
		
		virtual Promise<ArrayList<ScrobbleResponse>> scrobble(ArrayList<ScrobbleRequest> scrobbles) override;
		
		virtual Promise<void> loveTrack(String track, String artist) override;
		virtual Promise<void> unloveTrack(String track, String artist) override;
		
	private:
		static ScrobbleIgnored::Code ignoredScrobbleCodeFromString(const String&);
		
		LastFM* lastfm;
	};
}

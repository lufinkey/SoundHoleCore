//
//  MediaMatcher.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/21/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "MediaProvider.hpp"
#include "OmniTrack.hpp"
#include "OmniArtist.hpp"

namespace sh {
	class OmniTrack;
	class OmniArtist;

	class MediaMatcher: public virtual MediaProvider {
	public:
		virtual Promise<ArrayList<$<Track>>> findEquivalentTracks($<Track>) = 0;
		virtual Promise<ArrayList<$<Artist>>> findEquivalentArtists($<Artist>) = 0;
		
		virtual $<OmniTrack> omniTrack(const OmniTrack::Data& data);
		virtual $<OmniArtist> omniArtist(const OmniArtist::Data& data);
	};
}

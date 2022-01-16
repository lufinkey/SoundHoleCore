//
//  Scrobbler.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/6/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "Track.hpp"
#include "Scrobble.hpp"

namespace sh {
	class Scrobbler {
	public:
		virtual ~Scrobbler() {}
		
		virtual String name() const = 0;
		virtual String displayName() const = 0;
		
		virtual Promise<bool> login() = 0;
		virtual void logout() = 0;
		virtual bool isLoggedIn() const = 0;
		
		virtual Promise<ArrayList<Scrobble::Response>> scrobble(ArrayList<$<Scrobble>> scrobbles) = 0;
		
		virtual Promise<void> loveTrack($<Track>) = 0;
		virtual Promise<void> unloveTrack($<Track>) = 0;
	};
}

//
//  ScrobblerStash.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/15/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class Scrobbler;
	class Scrobble;

	class ScrobblerStash {
	public:
		virtual ~ScrobblerStash() {}
		
		virtual Scrobbler* getScrobbler(const String& name) = 0;
		virtual ArrayList<Scrobbler*> getScrobblers() = 0;
	};
}

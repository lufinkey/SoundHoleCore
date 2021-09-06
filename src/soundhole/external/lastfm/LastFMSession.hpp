//
//  LastFMSession.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/5/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class LastFMSession {
	public:
		String name;
		bool subscriber;
		String key;
		
		static LastFMSession fromJson(const Json& json);
		Json toJson() const;
	};
}

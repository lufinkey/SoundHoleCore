//
//  SpotifyProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/providers/MediaProvider.hpp>

namespace sh {
	class SpotifyProvider: public MediaProvider {
	public:
		virtual String getName() const override;
		virtual String getDisplayName() const override;
	};
}

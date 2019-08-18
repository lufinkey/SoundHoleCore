//
//  BandcampProvider.hpp
//  SoundHoleCore-macOS
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/providers/MediaProvider.hpp>

namespace sh {
	class BandcampProvider: public MediaProvider {
	public:
		virtual String getName() const override;
		virtual String getDisplayName() const override;
	};
}

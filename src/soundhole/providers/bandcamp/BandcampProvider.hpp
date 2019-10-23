//
//  BandcampProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <memory>
#include <soundhole/providers/MediaProvider.hpp>
#include "api/Bandcamp.hpp"

namespace sh {
	class BandcampProvider: public MediaProvider {
	public:
		BandcampProvider();
		virtual ~BandcampProvider();
		
		virtual String getName() const override;
		virtual String getDisplayName() const override;
		
	private:
		Bandcamp* bandcamp;
	};
}

//
//  BandcampProvider.cpp
//  SoundHoleCore-macOS
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "BandcampProvider.hpp"

namespace sh {
	String BandcampProvider::getName() const {
		return "bandcamp";
	}
	
	String BandcampProvider::getDisplayName() const {
		return "Bandcamp";
	}
}

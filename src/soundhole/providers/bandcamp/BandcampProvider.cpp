//
//  BandcampProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "BandcampProvider.hpp"

namespace sh {
	BandcampProvider::BandcampProvider()
	: bandcamp(new Bandcamp()) {
		//
	}

	BandcampProvider::~BandcampProvider() {
		delete bandcamp;
	}

	String BandcampProvider::getName() const {
		return "bandcamp";
	}
	
	String BandcampProvider::getDisplayName() const {
		return "Bandcamp";
	}
}

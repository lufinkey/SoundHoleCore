//
//  YoutubeProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/23/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "YoutubeProvider.hpp"

namespace sh {
	YoutubeProvider::YoutubeProvider() {
		//
	}
	
	String YoutubeProvider::name() const {
		return "youtube";
	}
	
	String YoutubeProvider::displayName() const {
		return "Youtube";
	}
}

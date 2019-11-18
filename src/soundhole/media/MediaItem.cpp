//
//  MediaItem.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "MediaItem.hpp"

namespace sh {
	MediaItem::MediaItem(MediaProvider* provider)
	: provider(provider) {
		//
	}
	
	MediaItem::~MediaItem() {
		//
	}
	
	MediaProvider* MediaItem::getProvider() const {
		return provider;
	}

	bool MediaItem::needsData() const {
		return false;
	}

	Promise<void> MediaItem::fetchMissingData() {
		return Promise<void>::resolve();
	}
}

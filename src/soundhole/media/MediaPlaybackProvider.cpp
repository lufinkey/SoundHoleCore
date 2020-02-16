//
//  MediaPlaybackProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/1/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "MediaPlaybackProvider.hpp"

namespace sh {
	void MediaPlaybackProvider::addEventListener(EventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		listeners.pushBack(listener);
	}

	void MediaPlaybackProvider::removeEventListener(EventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		listeners.removeLastEqual(listener);
	}
}

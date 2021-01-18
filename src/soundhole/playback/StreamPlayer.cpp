//
//  StreamPlayer.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/8/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "StreamPlayer.hpp"

namespace sh {
	void StreamPlayer::addListener(Listener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		this->listeners.pushBack(listener);
		lock.unlock();
	}

	void StreamPlayer::removeListener(Listener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		this->listeners.removeLastEqual(listener);
		lock.unlock();
	}

	String StreamPlayer::getAudioURL() const {
		std::unique_lock<std::recursive_mutex> lock(playerMutex);
		String audioURL = playerAudioURL;
		lock.unlock();
		return audioURL;
	}
}

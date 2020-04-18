//
//  Player_objc.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 4/18/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "Player.hpp"
#include "Player_objc.hpp"
#include "Player_objc_private.hpp"

#ifdef __OBJC__

namespace sh {
	SHPlayerEventListenerWrapper::SHPlayerEventListenerWrapper(id<SHPlayerEventListener> listener)
	: objcListener(listener) {
		//
	}
	
	id<SHPlayerEventListener> SHPlayerEventListenerWrapper::getObjCListener() const {
		return objcListener;
	}
	
	void SHPlayerEventListenerWrapper::onPlayerStateChange($<Player> player, const Player::Event& event) {
		if([objcListener respondsToSelector:@selector(player:didChangeState:)]) {
			[objcListener player:player didChangeState:event];
		}
	}
	
	void SHPlayerEventListenerWrapper::onPlayerMetadataChange($<Player> player, const Player::Event& event) {
		if([objcListener respondsToSelector:@selector(player:didChangeMetadata:)]) {
			[objcListener player:player didChangeMetadata:event];
		}
	}
	
	void SHPlayerEventListenerWrapper::onPlayerQueueChange($<Player> player, const Player::Event& event) {
		if([objcListener respondsToSelector:@selector(player:didChangeQueue:)]) {
			[objcListener player:player didChangeQueue:event];
		}
	}
	
	void SHPlayerEventListenerWrapper::onPlayerTrackFinish($<Player> player, const Player::Event& event) {
		if([objcListener respondsToSelector:@selector(player:didFinishTrack:)]) {
			[objcListener player:player didFinishTrack:event];
		}
	}
	
	void SHPlayerEventListenerWrapper::onPlayerPlay($<Player> player, const Player::Event& event) {
		if([objcListener respondsToSelector:@selector(player:didPlay:)]) {
			[objcListener player:player didPlay:event];
		}
	}
	
	void SHPlayerEventListenerWrapper::onPlayerPause($<Player> player, const Player::Event& event) {
		if([objcListener respondsToSelector:@selector(player:didPause:)]) {
			[objcListener player:player didPause:event];
		}
	}
	
	void Player::addEventListener(id<SHPlayerEventListener> listener) {
		auto listenerWrapper = new SHPlayerEventListenerWrapper(listener);
		addEventListener(listenerWrapper);
	}
	
	void Player::removeEventListener(id<SHPlayerEventListener> listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		auto listenerWrapper = listeners.firstWhere([&](auto& cmpListener) {
			if(auto cmpListenerWrapper = dynamic_cast<SHPlayerEventListenerWrapper*>(cmpListener)) {
				return (cmpListenerWrapper->getObjCListener() == listener);
			}
			return false;
		}, nullptr);
		if(listenerWrapper == nullptr) {
			return;
		}
		lock.unlock();
		removeEventListener(listenerWrapper);
		delete listenerWrapper;
	}
	
	void Player::deleteObjcListenerWrappers() {
		for(auto listener : listeners) {
			if(auto listenerWrapper = dynamic_cast<SHPlayerEventListenerWrapper*>(listener)) {
				delete listenerWrapper;
			}
		}
	}
}

#endif

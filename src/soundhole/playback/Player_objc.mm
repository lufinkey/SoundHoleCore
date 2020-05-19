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

#if defined(TARGETPLATFORM_IOS)
#import <AVFoundation/AVFoundation.h>
#endif

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
		auto listenerWrapperIt = listeners.findWhere([&](auto& cmpListener) {
			if(auto cmpListenerWrapper = dynamic_cast<SHPlayerEventListenerWrapper*>(cmpListener)) {
				return (cmpListenerWrapper->getObjCListener() == listener);
			}
			return false;
		});
		if(listenerWrapperIt == listeners.end()) {
			return;
		}
		auto listenerWrapper = *listenerWrapperIt;
		listeners.erase(listenerWrapperIt);
		delete listenerWrapper;
	}

	bool Player::hasEventListener(id<SHPlayerEventListener> listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		return listeners.containsWhere([&](auto cmpListener) {
			if(auto cmpListenerWrapper = dynamic_cast<SHPlayerEventListenerWrapper*>(cmpListener)) {
				return (cmpListenerWrapper->getObjCListener() == listener);
			}
			return false;
		});
	}
	
	void Player::deleteObjcListenerWrappers() {
		for(auto listener : listeners) {
			if(auto listenerWrapper = dynamic_cast<SHPlayerEventListenerWrapper*>(listener)) {
				delete listenerWrapper;
			}
		}
	}


	#if defined(TARGETPLATFORM_IOS)

	void Player::activateAudioSession() {
		AVAudioSession* audioSession = [AVAudioSession sharedInstance];
		NSError* error = nil;
		NSString* audioSessionCategory = AVAudioSessionCategoryPlayback;
		if(![audioSessionCategory isEqualToString:audioSession.category]) {
			[audioSession setCategory:audioSessionCategory error:&error];
			if(error != nil) {
				printf("Error setting spotify audio session category: %s\n", error.description.UTF8String);
			}
		}
		error = nil;
		[audioSession setActive:YES error:&error];
		if(error != nil) {
			printf("Error setting spotify audio session active: %s\n", error.description.UTF8String);
		}
	}

	void Player::deactivateAudioSession() {
		AVAudioSession* audioSession = [AVAudioSession sharedInstance];
		NSError* error = nil;
		[audioSession setActive:NO error:&error];
		if(error != nil) {
			printf("Error setting spotify audio session inactive: %s\n", error.description.UTF8String);
		}
	}

	#endif
}

#endif

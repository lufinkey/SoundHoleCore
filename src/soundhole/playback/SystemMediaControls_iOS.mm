//
//  SystemMediaControls_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 10/23/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "SystemMediaControls_iOS.h"
#include "SystemMediaControls.hpp"

@implementation SHSystemMediaControlsListenerWrapper

-(id)initWithListener:(sh::SystemMediaControls::Listener*)listener {
	if(self = [super init]) {
		_listener = listener;
	}
	return self;
}

+(MPRemoteCommandHandlerStatus)nativeHandlerStatusFrom:(sh::SystemMediaControls::HandlerStatus)handlerStatus {
	using HandlerStatus = sh::SystemMediaControls::HandlerStatus;
	switch(handlerStatus) {
		case HandlerStatus::SUCCESS:
			return MPRemoteCommandHandlerStatusSuccess;
		case HandlerStatus::FAILED:
			return MPRemoteCommandHandlerStatusCommandFailed;
		case HandlerStatus::NO_SUCH_CONTENT:
			return MPRemoteCommandHandlerStatusNoSuchContent;
		case HandlerStatus::NO_NOW_PLAYING_ITEM:
			return MPRemoteCommandHandlerStatusNoActionableNowPlayingItem;
		case HandlerStatus::DEVICE_NOT_FOUND:
			return MPRemoteCommandHandlerStatusDeviceNotFound;
	}
	return MPRemoteCommandHandlerStatusCommandFailed;
}

#define nativeHandlerStatus(handlerStatus) [self.class nativeHandlerStatusFrom:handlerStatus]

+(sh::SystemMediaControls::RepeatMode)repeatModeFrom:(MPRepeatType)repeatType {
	using RepeatMode = sh::SystemMediaControls::RepeatMode;
	switch(repeatType) {
		case MPRepeatTypeOff:
			return RepeatMode::OFF;
		case MPRepeatTypeOne:
			return RepeatMode::ONE;
		case MPRepeatTypeAll:
			return RepeatMode::ALL;
	}
	return RepeatMode::OFF;
}

+(MPRepeatType)nativeRepeatModeFrom:(sh::SystemMediaControls::RepeatMode)repeatMode {
	using RepeatMode = sh::SystemMediaControls::RepeatMode;
	switch(repeatMode) {
		case RepeatMode::OFF:
			return MPRepeatTypeOff;
		case RepeatMode::ONE:
			return MPRepeatTypeOne;
		case RepeatMode::ALL:
			return MPRepeatTypeAll;
	}
	return MPRepeatTypeOff;
}

+(bool)shuffleModeFrom:(MPShuffleType)shuffleType {
	switch(shuffleType) {
		case MPShuffleTypeOff:
			return false;
		case MPShuffleTypeItems:
			return true;
		case MPShuffleTypeCollections:
			return true;
	}
}

+(MPShuffleType)nativeShuffleModeFrom:(bool)shuffleMode {
	if(shuffleMode) {
		return MPShuffleTypeItems;
	} else {
		return MPShuffleTypeOff;
	}
}

+(sh::SystemMediaControls::PlaybackState)playbackStateFrom:(MPNowPlayingPlaybackState)state {
	using PlaybackState = sh::SystemMediaControls::PlaybackState;
	switch(state) {
		case MPNowPlayingPlaybackStatePlaying:
			return PlaybackState::PLAYING;
		case MPNowPlayingPlaybackStatePaused:
			return PlaybackState::PAUSED;
		case MPNowPlayingPlaybackStateStopped:
			return PlaybackState::STOPPED;
		case MPNowPlayingPlaybackStateUnknown:
			return PlaybackState::UNKNOWN;
		case MPNowPlayingPlaybackStateInterrupted:
			return PlaybackState::INTERRUPTED;
	}
	return PlaybackState::UNKNOWN;
}

+(MPNowPlayingPlaybackState)nativePlaybackStateFrom:(sh::SystemMediaControls::PlaybackState)state {
	using PlaybackState = sh::SystemMediaControls::PlaybackState;
	switch(state) {
		case PlaybackState::PLAYING:
			return MPNowPlayingPlaybackStatePlaying;
		case PlaybackState::PAUSED:
			return MPNowPlayingPlaybackStatePaused;
		case PlaybackState::STOPPED:
			return MPNowPlayingPlaybackStateStopped;
		case PlaybackState::UNKNOWN:
			return MPNowPlayingPlaybackStateUnknown;
		case PlaybackState::INTERRUPTED:
			return MPNowPlayingPlaybackStateInterrupted;
	}
	return MPNowPlayingPlaybackStateUnknown;
}

-(void)subscribeTo:(MPRemoteCommandCenter*)commandCenter {
	[self unsubscribeFrom:commandCenter];
	[commandCenter.playCommand addTarget:self action:@selector(remoteCommandCenterDidPlay:)];
	[commandCenter.pauseCommand addTarget:self action:@selector(remoteCommandCenterDidPause:)];
	[commandCenter.stopCommand addTarget:self action:@selector(remoteCommandCenterDidStop:)];
	[commandCenter.previousTrackCommand addTarget:self action:@selector(remoteCommandCenterDidGoToPrevious:)];
	[commandCenter.nextTrackCommand addTarget:self action:@selector(remoteCommandCenterDidGoToNext:)];
	[commandCenter.changeRepeatModeCommand addTarget:self action:@selector(remoteCommandCenterDidChangeRepeatMode:)];
	[commandCenter.changeShuffleModeCommand addTarget:self action:@selector(remoteCommandCenterDidChangeShuffleMode:)];
}

-(void)unsubscribeFrom:(MPRemoteCommandCenter*)commandCenter {
	[commandCenter.playCommand removeTarget:self];
	[commandCenter.pauseCommand removeTarget:self];
	[commandCenter.stopCommand removeTarget:self];
	[commandCenter.previousTrackCommand removeTarget:self];
	[commandCenter.nextTrackCommand removeTarget:self];
	[commandCenter.changeRepeatModeCommand removeTarget:self];
	[commandCenter.changeShuffleModeCommand removeTarget:self];
}

-(MPRemoteCommandHandlerStatus)remoteCommandCenterDidPlay:(MPRemoteCommandEvent*)event {
	return nativeHandlerStatus(_listener->onMediaControlsPlay());
}

-(MPRemoteCommandHandlerStatus)remoteCommandCenterDidPause:(MPRemoteCommandEvent*)event {
	return nativeHandlerStatus(_listener->onMediaControlsPause());
}

-(MPRemoteCommandHandlerStatus)remoteCommandCenterDidStop:(MPRemoteCommandEvent*)event {
	return nativeHandlerStatus(_listener->onMediaControlsStop());
}

-(MPRemoteCommandHandlerStatus)remoteCommandCenterDidGoToPrevious:(MPRemoteCommandEvent*)event {
	return nativeHandlerStatus(_listener->onMediaControlsPrevious());
}

-(MPRemoteCommandHandlerStatus)remoteCommandCenterDidGoToNext:(MPRemoteCommandEvent*)event {
	return nativeHandlerStatus(_listener->onMediaControlsNext());
}

-(MPRemoteCommandHandlerStatus)remoteCommandCenterDidChangeRepeatMode:(MPRemoteCommandEvent*)event {
	MPChangeRepeatModeCommand* command = (MPChangeRepeatModeCommand*)event.command;
	auto repeatMode = [self.class repeatModeFrom:command.currentRepeatType];
	return nativeHandlerStatus(_listener->onMediaControlsChangeRepeatMode(repeatMode));
}

-(MPRemoteCommandHandlerStatus)remoteCommandCenterDidChangeShuffleMode:(MPRemoteCommandEvent*)event {
	MPChangeShuffleModeCommand* command = (MPChangeShuffleModeCommand*)event.command;
	auto shuffleMode = [self.class shuffleModeFrom:command.currentShuffleType];
	return nativeHandlerStatus(_listener->onMediaControlsChangeShuffleMode(shuffleMode));
}

@end



namespace sh {
	struct _SystemMediaControlsNativeData {
		MPRemoteCommandCenter* remoteCommandCenter;
		MPNowPlayingInfoCenter* nowPlayingInfoCenter;
	};
	
	struct _SystemMediaControlsListenerNode {
		SHSystemMediaControlsListenerWrapper* wrapper;
	};
	
	SystemMediaControls::SystemMediaControls() {
		nativeData = new _SystemMediaControlsNativeData{
			.remoteCommandCenter = MPRemoteCommandCenter.sharedCommandCenter,
			.nowPlayingInfoCenter = MPNowPlayingInfoCenter.defaultCenter
		};
	}
	
	SystemMediaControls::~SystemMediaControls() {
		delete nativeData;
	}
	
	void SystemMediaControls::addListener(Listener* listener) {
		SHSystemMediaControlsListenerWrapper* wrapper = [[SHSystemMediaControlsListenerWrapper alloc] initWithListener:listener];
		listenerNodes.pushBack(new _SystemMediaControlsListenerNode{
			.wrapper=wrapper
		});
		[wrapper subscribeTo:nativeData->remoteCommandCenter];
	}
	
	void SystemMediaControls::removeListener(Listener* listener) {
		auto it = listenerNodes.findLastWhere([=](auto& node) {
			return (node->wrapper.listener == listener);
		});
		if(it != listenerNodes.end()) {
			auto node = *it;
			[node->wrapper unsubscribeFrom:nativeData->remoteCommandCenter];
			listenerNodes.erase(it);
			delete node;
		}
	}
	
	void SystemMediaControls::setButtonState(ButtonState state) {
		MPRemoteCommandCenter* commandCenter = nativeData->remoteCommandCenter;
		commandCenter.playCommand.enabled = state.playEnabled;
		commandCenter.pauseCommand.enabled = state.pauseEnabled;
		commandCenter.stopCommand.enabled = state.stopEnabled;
		commandCenter.previousTrackCommand.enabled = state.previousEnabled;
		commandCenter.nextTrackCommand.enabled = state.nextEnabled;
		commandCenter.changeRepeatModeCommand.currentRepeatType = [SHSystemMediaControlsListenerWrapper nativeRepeatModeFrom:state.repeatMode];
		commandCenter.changeShuffleModeCommand.currentShuffleType = [SHSystemMediaControlsListenerWrapper nativeShuffleModeFrom:state.shuffleMode];
	}
	
	SystemMediaControls::ButtonState SystemMediaControls::getButtonState() const {
		MPRemoteCommandCenter* commandCenter = nativeData->remoteCommandCenter;
		return SystemMediaControls::ButtonState{
			.playEnabled = commandCenter.playCommand.enabled,
			.pauseEnabled = commandCenter.pauseCommand.enabled,
			.stopEnabled = commandCenter.stopCommand.enabled,
			.previousEnabled = commandCenter.previousTrackCommand.enabled,
			.nextEnabled = commandCenter.nextTrackCommand.enabled,
			.repeatEnabled = commandCenter.changeRepeatModeCommand.enabled,
			.repeatMode = [SHSystemMediaControlsListenerWrapper repeatModeFrom:commandCenter.changeRepeatModeCommand.currentRepeatType],
			.shuffleEnabled = commandCenter.changeShuffleModeCommand.enabled,
			.shuffleMode = [SHSystemMediaControlsListenerWrapper shuffleModeFrom:commandCenter.changeShuffleModeCommand.currentShuffleType]
		};
	}
	
	void SystemMediaControls::setPlaybackState(PlaybackState state) {
		if (@available(iOS 13.0, *)) {
			nativeData->nowPlayingInfoCenter.playbackState = [SHSystemMediaControlsListenerWrapper nativePlaybackStateFrom:state];
		}
	}
	
	SystemMediaControls::PlaybackState SystemMediaControls::getPlaybackState() const {
		if (@available(iOS 13.0, *)) {
			return [SHSystemMediaControlsListenerWrapper playbackStateFrom:nativeData->nowPlayingInfoCenter.playbackState];
		} else {
			return PlaybackState::UNKNOWN;
		}
	}
	
	void SystemMediaControls::setNowPlaying(NowPlaying nowPlaying) {
		NSMutableDictionary* dict = [NSMutableDictionary dictionary];
		if(nowPlaying.title) {
			dict[MPMediaItemPropertyTitle] = nowPlaying.title->toNSString();
		}
		if(nowPlaying.artist) {
			dict[MPMediaItemPropertyArtist] = nowPlaying.artist->toNSString();
		}
		if(nowPlaying.albumTitle) {
			dict[MPMediaItemPropertyAlbumTitle] = nowPlaying.albumTitle->toNSString();
		}
		if(nowPlaying.trackIndex) {
			dict[MPMediaItemPropertyAlbumTrackNumber] = @((NSUInteger)nowPlaying.trackIndex.value());
		}
		if(nowPlaying.trackCount) {
			dict[MPMediaItemPropertyAlbumTrackCount] = @((NSUInteger)nowPlaying.trackCount.value());
		}
		if(nowPlaying.elapsedTime) {
			dict[MPNowPlayingInfoPropertyElapsedPlaybackTime] = @((NSTimeInterval)nowPlaying.elapsedTime.value());
		}
		if(nowPlaying.duration) {
			dict[MPMediaItemPropertyPlaybackDuration] = @((NSTimeInterval)nowPlaying.duration.value());
		}
		nativeData->nowPlayingInfoCenter.nowPlayingInfo = dict;
	}
	
	SystemMediaControls::NowPlaying SystemMediaControls::getNowPlaying() const {
		NSDictionary* dict = nativeData->nowPlayingInfoCenter.nowPlayingInfo;
		NowPlaying nowPlaying;
		if(NSString* title = dict[MPMediaItemPropertyTitle]) {
			nowPlaying.title = String(title);
		}
		if(NSString* artist = dict[MPMediaItemPropertyArtist]) {
			nowPlaying.artist = String(artist);
		}
		if(NSString* albumTitle = dict[MPMediaItemPropertyAlbumTitle]) {
			nowPlaying.albumTitle = String(albumTitle);
		}
		if(NSNumber* trackIndex = dict[MPMediaItemPropertyAlbumTrackNumber]) {
			nowPlaying.trackIndex = (size_t)trackIndex.unsignedIntegerValue;
		}
		if(NSNumber* trackCount = dict[MPMediaItemPropertyAlbumTrackCount]) {
			nowPlaying.trackCount = (size_t)trackCount.unsignedIntegerValue;
		}
		if(NSNumber* elapsedTime = dict[MPNowPlayingInfoPropertyElapsedPlaybackTime]) {
			nowPlaying.elapsedTime = elapsedTime.doubleValue;
		}
		if(NSNumber* duration = dict[MPMediaItemPropertyPlaybackDuration]) {
			nowPlaying.duration = duration.doubleValue;
		}
		return nowPlaying;
	}
}

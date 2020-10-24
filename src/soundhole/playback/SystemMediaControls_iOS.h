//
//  SystemMediaControls_iOS.h
//  SoundHoleCore
//
//  Created by Luis Finke on 10/23/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <MediaPlayer/MediaPlayer.h>
#include "SystemMediaControls.hpp"

@interface SHSystemMediaControlsListenerWrapper: NSObject

-(id)initWithListener:(sh::SystemMediaControls::Listener*)listener;

+(MPRemoteCommandHandlerStatus)nativeHandlerStatusFrom:(sh::SystemMediaControls::Listener::HandlerStatus)handlerStatus;
+(sh::SystemMediaControls::RepeatMode)repeatModeFrom:(MPRepeatType)repeatType;
+(MPRepeatType)nativeRepeatModeFrom:(sh::SystemMediaControls::RepeatMode)repeatMode;
+(bool)shuffleModeFrom:(MPShuffleType)shuffleType;
+(MPShuffleType)nativeShuffleModeFrom:(bool)shuffleMode;
+(sh::SystemMediaControls::PlaybackState)playbackStateFrom:(MPNowPlayingPlaybackState)state;
+(MPNowPlayingPlaybackState)nativePlaybackStateFrom:(sh::SystemMediaControls::PlaybackState)state;

-(void)subscribeTo:(MPRemoteCommandCenter*)commandCenter;
-(void)unsubscribeFrom:(MPRemoteCommandCenter*)commandCenter;

@property (nonatomic,readonly) sh::SystemMediaControls::Listener* listener;

@end

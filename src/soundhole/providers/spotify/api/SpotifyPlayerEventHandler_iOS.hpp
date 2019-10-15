//
//  SpotifyPlayerEventHandler_iOS.hpp
//  SoundHoleCore-iOS
//
//  Created by Luis Finke on 9/23/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <Foundation/Foundation.h>
#import <SpotifyAudioPlayback/SpotifyAudioPlayback.h>

NS_ASSUME_NONNULL_BEGIN

@interface SpotifyPlayerEventHandler: NSObject<SPTAudioStreamingDelegate,SPTAudioStreamingPlaybackDelegate>

@property void(^onError)(NSError*);
@property void(^onLogin)();
@property void(^onLogout)();
@property void(^onTemporaryConnectionError)();
@property void(^onMessage)(NSString*);
@property void(^onDisconnect)();
@property void(^onReconnect)();
@property void(^onPlaybackEvent)(SpPlaybackEvent);
@property void(^onChangePlaybackStatus)(BOOL);

@end

NS_ASSUME_NONNULL_END

#endif

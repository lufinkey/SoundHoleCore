//
//  StreamPlayerEventHandler_iOS.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/12/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface StreamPlayerEventHandler: NSObject

-(void)playerItemDidReachEnd:(NSNotification*)notification;
-(void)observeValueForKeyPath:(nullable NSString*)keyPath ofObject:(nullable id)object change:(nullable NSDictionary<NSKeyValueChangeKey,id>*)change context:(nullable void*)context;

@property void(^onItemFinish)(AVPlayerItem*);
@property void(^onPlay)(void);
@property void(^onPause)(void);
@property void(^onBuffering)(void);
@property void(^onContinue)(void);

@end

NS_ASSUME_NONNULL_END

#endif

//
//  Player_objc.h
//  SoundHoleCore
//
//  Created by Luis Finke on 4/18/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "Player.hpp"
#ifdef __OBJC__
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol SHPlayerEventListener <NSObject>

@optional
-(void)player:(fgl::$<sh::Player>)player didChangeState:(const sh::Player::Event&)event;
-(void)player:(fgl::$<sh::Player>)player didChangeMetadata:(const sh::Player::Event&)event;
-(void)player:(fgl::$<sh::Player>)player didChangeQueue:(const sh::Player::Event&)event;
-(void)player:(fgl::$<sh::Player>)player didFinishTrack:(const sh::Player::Event&)event;

-(void)player:(fgl::$<sh::Player>)player didPlay:(const sh::Player::Event&)event;
-(void)player:(fgl::$<sh::Player>)player didPause:(const sh::Player::Event&)event;

@end

NS_ASSUME_NONNULL_END

#endif

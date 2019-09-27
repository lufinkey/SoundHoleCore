//
//  SpotifyAuthViewController_iOS.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/27/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <optional>
#include <soundhole/common.hpp>
#include "SpotifyAuth.hpp"
#include "SpotifyError.hpp"
#include "SpotifySession.hpp"
#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

typedef void(^SpotifyAuthViewControllerCompletion)(sh::Optional<sh::SpotifySession> session,std::exception_ptr error);

@interface SpotifyAuthViewController: UINavigationController

-(id)initWithOptions:(sh::SpotifyAuth::Options)options;

@property (nullable) SpotifyAuthViewControllerCompletion completion;

@property (readonly, class) UIViewController* topVisibleViewController;

@end

NS_ASSUME_NONNULL_END

#endif

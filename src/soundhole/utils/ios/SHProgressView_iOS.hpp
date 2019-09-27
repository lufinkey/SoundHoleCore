//
//  SHProgressView.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/27/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface SHProgressView: UIView

-(void)showInView:(UIView*)view animated:(BOOL)animated completion:(nullable void(^)(void))completion;
-(void)dismissAnimated:(BOOL)animated completion:(nullable void(^)(void))completion;

@property (readonly) UIActivityIndicatorView* activityIndicator;

@end

NS_ASSUME_NONNULL_END

#endif

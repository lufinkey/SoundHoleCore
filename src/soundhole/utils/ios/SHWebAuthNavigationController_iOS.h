//
//  SHWebAuthNavigationController_iOS.h
//  SoundHoleCore
//
//  Created by Luis Finke on 9/13/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import "SHWebViewController_iOS.h"

NS_ASSUME_NONNULL_BEGIN

@class SHWebAuthNavigationController;
typedef BOOL(^SHWebAuthNavigationControllerRedirectHandler)(SHWebAuthNavigationController* controller, WKWebView* webView, WKNavigationAction* action);

@interface SHWebAuthNavigationController: UINavigationController <WKNavigationDelegate, UIAdaptivePresentationControllerDelegate>

-(id)init;

@property (nonatomic, readonly) SHWebViewController* webViewController;
@property (nonatomic, nullable) SHWebAuthNavigationControllerRedirectHandler onWebRedirect;
@property (nonatomic, nullable) void(^onCancel)(void);
@property (nonatomic) BOOL loadingOverlayVisible;
-(void)setLoadingOverlayVisible:(BOOL)loadingOverlayVisible animated:(BOOL)animated;

@property (nonatomic) UIStatusBarStyle statusBarStyle;

@end

NS_ASSUME_NONNULL_END

#endif

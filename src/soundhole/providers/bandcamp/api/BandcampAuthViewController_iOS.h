//
//  BandcampAuthViewController_iOS.h
//  SoundHoleCore
//
//  Created by Luis Finke on 11/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include <soundhole/common.hpp>
#include "BandcampSession.hpp"
#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <soundhole/utils/ios/SHWebViewController_iOS.h>

NS_ASSUME_NONNULL_BEGIN

@interface BandcampAuthViewController: UINavigationController <WKNavigationDelegate, UIAdaptivePresentationControllerDelegate>

@property (nonatomic, readonly) SHWebViewController* webViewController;

@property (nonatomic, nullable) void(^onCancel)();
@property (nonatomic, nullable) void(^onSuccess)(BandcampAuthViewController*,sh::BandcampSession);

@property (nonatomic) BOOL loadingOverlayVisible;
-(void)setLoadingOverlayVisible:(BOOL)loadingOverlayVisible animated:(BOOL)animated;

@property (nonatomic) UIStatusBarStyle statusBarStyle;

@end

NS_ASSUME_NONNULL_END

#endif

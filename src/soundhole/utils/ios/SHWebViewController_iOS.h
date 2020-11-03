//
//  SHWebViewController_iOS.h
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
#import <WebKit/WebKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface SHWebViewController: UIViewController

-(id)init;
-(id)initWithConfiguration:(WKWebViewConfiguration*)configuration;

@property (readonly) WKWebView* webView;

@end

NS_ASSUME_NONNULL_END

#endif

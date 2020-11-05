//
//  BandcampAuth_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 11/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "BandcampAuth.hpp"
#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import "BandcampAuthViewController_iOS.h"
#import <soundhole/utils/ios/SHiOSUtils.h>
#import <soundhole/utils/objc/SHObjcUtils.h>
#import <WebKit/WebKit.h>

namespace sh {
	Promise<Optional<BandcampSession>> BandcampAuth::authenticate() {
		return Promise<Optional<BandcampSession>>([=](auto resolve, auto reject) {
			dispatch_async(dispatch_get_main_queue(), ^{
				BandcampAuthViewController* viewController = [[BandcampAuthViewController alloc] init];
				viewController.onSuccess = ^(BandcampAuthViewController* viewController, NSArray<NSHTTPCookie*>* cookies) {
					ArrayList<String> cookiesArray;
					cookiesArray.reserve((size_t)cookies.count);
					for(NSHTTPCookie* cookie in cookies) {
						cookiesArray.pushBack(String([SHObjcUtils stringFromCookie:cookie]));
					}
					[viewController setLoadingOverlayVisible:YES animated:YES];
					createBandcampSessionFromCookies(cookiesArray).then([=](auto session) {
						dispatch_async(dispatch_get_main_queue(), ^{
							[viewController setLoadingOverlayVisible:NO animated:YES];
							[viewController.presentingViewController dismissViewControllerAnimated:YES completion:^{
								if(session) {
									resolve(session.value());
								} else {
									reject(std::runtime_error("Couldn't create bandcamp session from cookies"));
								}
							}];
						});
					}, reject);
				};
				viewController.onCancel = ^{
					resolve(std::nullopt);
				};
				
				[viewController loadViewIfNeeded];
				[viewController.webViewController loadViewIfNeeded];
				
				// present auth controller
				UIViewController* topViewController = [SHiOSUtils topVisibleViewController];
				[topViewController presentViewController:viewController animated:YES completion:nil];
			});
		});
	}
}

#endif

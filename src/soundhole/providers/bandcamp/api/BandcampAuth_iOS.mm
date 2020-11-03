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
	Promise<bool> BandcampAuth::login() {
		return Promise<bool>([=](auto resolve, auto reject) {
			dispatch_async(dispatch_get_main_queue(), ^{
				BandcampAuthViewController* viewController = [[BandcampAuthViewController alloc] init];
				viewController.onSuccess = ^(BandcampAuthViewController* viewController, sh::BandcampSession session) {
					[viewController.presentingViewController dismissViewControllerAnimated:YES completion:^{
						loginWithSession(session);
						resolve(isLoggedIn());
					}];
				};
				viewController.onCancel = ^{
					resolve(false);
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

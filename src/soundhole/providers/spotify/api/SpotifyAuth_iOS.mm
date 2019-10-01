//
//  SpotifyAuth_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 9/22/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SpotifyAuth.hpp"

#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <Foundation/Foundation.h>
#include "SpotifyAuthViewController_iOS.hpp"

using namespace sh;

namespace sh {
	Promise<bool> SpotifyAuth::login(LoginOptions options) {
		return Promise<bool>([=](auto resolve, auto reject) {
			auto authOptions = this->options;
			authOptions.params["show_dialog"] = options.showDialog ? "true" : "false";
			SpotifyAuthViewController* authController = [[SpotifyAuthViewController alloc] initWithOptions:authOptions];
			__weak SpotifyAuthViewController* weakAuthController = authController;
			authController.completion = ^(Optional<SpotifySession> session, std::exception_ptr error) {
				SpotifyAuthViewController* authController = weakAuthController;
				auto finishLogin = [=]() {
					if(error) {
						reject(error);
					} else if(session) {
						loginWithSession(session.value());
						resolve(true);
					} else {
						resolve(false);
					}
				};
				if(authController.presentingViewController != nil) {
					dispatch_async(dispatch_get_main_queue(), ^{
						[authController.presentingViewController dismissViewControllerAnimated:YES completion:^{
							finishLogin();
						}];
					});
				} else {
					finishLogin();
				}
			};
			
			// present auth view controller
			UIViewController* topViewController = [SpotifyAuthViewController topVisibleViewController];
			[topViewController presentViewController:authController animated:YES completion:nil];
		});
	}
}

#endif

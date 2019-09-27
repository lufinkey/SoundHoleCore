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
	void SpotifyAuth::load() {
		if(options.sessionPersistKey.size() == 0) {
			return;
		}
		session = SpotifySession::fromNSUserDefaults(options.sessionPersistKey, NSUserDefaults.standardUserDefaults);
	}
	
	void SpotifyAuth::save() {
		if(options.sessionPersistKey.size() == 0) {
			return;
		}
		SpotifySession::writeToNSUserDefaults(session, options.sessionPersistKey, NSUserDefaults.standardUserDefaults);
	}
	
	
	Promise<bool> SpotifyAuth::login() {
		return Promise<bool>([=](auto resolve, auto reject) {
			SpotifyAuthViewController* authController = [[SpotifyAuthViewController alloc] initWithOptions:options];
			__weak SpotifyAuthViewController* weakAuthController = authController;
			authController.completion = ^(Optional<SpotifySession> session, std::exception_ptr error) {
				SpotifyAuthViewController* authController = weakAuthController;
				if(error) {
					// login failed
					dispatch_async(dispatch_get_main_queue(), ^{
						[authController.presentingViewController dismissViewControllerAnimated:YES completion:^{
							reject(error);
						}];
					});
				}
				else if(session.has_value()) {
					// login successful
					dispatch_async(dispatch_get_main_queue(), ^{
						[authController.presentingViewController dismissViewControllerAnimated:YES completion:^{
							startSession(session.value());
							resolve(true);
						}];
					});
				} else {
					// login cancelled
					dispatch_async(dispatch_get_main_queue(), ^{
						[authController.presentingViewController dismissViewControllerAnimated:YES completion:^{
							resolve(false);
						}];
					});
				}
			};
			
			// present auth view controller
			UIViewController* topViewController = [SpotifyAuthViewController topVisibleViewController];
			[topViewController presentViewController:authController animated:YES completion:nil];
		});
	}
}

#endif

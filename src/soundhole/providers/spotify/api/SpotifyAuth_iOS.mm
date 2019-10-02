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
	Promise<Optional<SpotifySession>> SpotifyAuth::authenticate(Options options) {
		if(options.params.find("show_dialog") == options.params.end()) {
			options.params["show_dialog"] = "true";
		}
		return Promise<Optional<SpotifySession>>([=](auto resolve, auto reject) {
			dispatch_async(dispatch_get_main_queue(), ^{
				SpotifyAuthViewController* authController = [[SpotifyAuthViewController alloc] initWithOptions:options];
				__weak SpotifyAuthViewController* weakAuthController = authController;
				authController.completion = ^(Optional<SpotifySession> session, std::exception_ptr error) {
					SpotifyAuthViewController* authController = weakAuthController;
					auto finishLogin = [=]() {
						if(error) {
							reject(error);
						} else {
							resolve(session);
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
		});
	}
}

#endif

//
//  SpotifyAuth_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 9/22/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SpotifyAuth.hpp"
#include "SpotifyError.hpp"

#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <Foundation/Foundation.h>
#import <soundhole/utils/HttpClient.hpp>
#import <soundhole/utils/ios/SHWebAuthNavigationController_iOS.h>
#import <soundhole/utils/ios/SHiOSUtils.h>

namespace sh {
	Promise<Optional<SpotifySession>> SpotifyAuth::authenticate(AuthenticateOptions options) {
		if(!options.showDialog.hasValue()) {
			options.showDialog = true;
		}
		return Promise<Optional<SpotifySession>>([=](auto resolve, auto reject) {
			dispatch_async(dispatch_get_main_queue(), ^{
				NSString* xssState = [NSUUID UUID].UUIDString;
				
				SHWebAuthNavigationController* authController = [[SHWebAuthNavigationController alloc] init];
				authController.handleNavigationAction = ^(SHWebAuthNavigationController* authNav, WKNavigationAction* action, void(^decisionHandler)(WKNavigationActionPolicy)) {
					// check if redirect URL matches
					NSURL* url = action.request.URL;
					if(!utils::checkURLMatch(options.redirectURL, String(url.absoluteString))) {
						// URL doesn't match redirect URL
						decisionHandler(WKNavigationActionPolicyAllow);
						return;
					}
					// URL matches redirectURL
					decisionHandler(WKNavigationActionPolicyCancel);
					// show loading overlay
					[authNav setLoadingOverlayVisible:YES animated:YES];
					// define function to dismiss controller
					auto dismissAuthController = [=](void(^completion)(void)) {
						// hide loading overlay
						[authNav setLoadingOverlayVisible:NO animated:YES];
						// dismiss controller
						dispatch_async(dispatch_get_main_queue(), ^{
							if(authNav.presentingViewController != nil) {
								[authNav.presentingViewController dismissViewControllerAnimated:YES completion:completion];
							} else {
								completion();
							}
						});
					};
					// handle oauth redirect
					auto params = utils::parseURLQueryParams(String(url.absoluteString));
					SpotifyAuth::handleOAuthRedirect(params, options, String(xssState)).then([=](Optional<SpotifySession> session) {
						dismissAuthController(^{
							resolve(session);
						});
					}).except([=](std::exception_ptr error) {
						dismissAuthController(^{
							reject(error);
						});
					});
				};
				authController.onCancel = ^{
					// cancel auth
					resolve(std::nullopt);
				};
				
				// prepare auth controller
				[authController loadViewIfNeeded];
				[authController.webViewController loadViewIfNeeded];
				NSURL* url = [NSURL URLWithString:options.getWebAuthenticationURL(String(xssState)).toNSString()];
				NSURLRequest* request = [NSURLRequest requestWithURL:url];
				[authController.webViewController.webView loadRequest:request];
				
				// present auth controller
				UIViewController* topViewController = [SHiOSUtils topVisibleViewController];
				[topViewController presentViewController:authController animated:YES completion:nil];
			});
		});
	}
}

#endif

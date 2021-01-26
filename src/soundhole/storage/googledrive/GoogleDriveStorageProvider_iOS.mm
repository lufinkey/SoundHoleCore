//
//  GoogleDriveStorageProvider_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 12/29/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "GoogleDriveStorageProvider.hpp"

#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <Foundation/Foundation.h>
#import <soundhole/utils/HttpClient.hpp>
#import <soundhole/utils/ios/SHWebAuthNavigationController_iOS.h>
#import <soundhole/utils/ios/SHiOSUtils.h>

namespace sh {
	Promise<bool> GoogleDriveStorageProvider::login() {
		return Promise<bool>([=](auto resolve, auto reject) {
			dispatch_async(dispatch_get_main_queue(), ^{
				String codeVerifier = String::random(64,
					"ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" "0123456789" "-._~");
				
				SHWebAuthNavigationController* authController = [[SHWebAuthNavigationController alloc] init];
				authController.onWebRedirect = ^BOOL(SHWebAuthNavigationController* authNav, WKWebView* webView, WKNavigationAction* action) {
					// check if redirect URL matches
					NSURL* url = action.request.URL;
					if(!utils::checkURLMatch(options.redirectURL, String(url.absoluteString))) {
						return NO;
					}
					[authNav setLoadingOverlayVisible:YES animated:YES];
					auto params = utils::parseURLQueryParams(String(url.absoluteString));
					auto dismissAuthController = [=](void(^completion)(void)) {
						[authNav setLoadingOverlayVisible:NO animated:YES];
						dispatch_async(dispatch_get_main_queue(), ^{
							if(authNav.presentingViewController != nil) {
								[authNav.presentingViewController dismissViewControllerAnimated:YES completion:completion];
							} else {
								completion();
							}
						});
					};
					handleOAuthRedirect(params, codeVerifier).then([=]() {
						dismissAuthController(^{
							resolve(true);
						});
					}).except([=](std::exception_ptr error) {
						dismissAuthController(^{
							reject(error);
						});
					});
					return YES;
				};
				authController.onCancel = ^{
					// cancel auth
					resolve(false);
				};
				
				// prepare auth controller
				[authController loadViewIfNeeded];
				[authController.webViewController loadViewIfNeeded];
				authController.webViewController.webView.customUserAgent = @"Mozilla/5.0";
				NSURL* url = [NSURL URLWithString:getWebAuthenticationURL(codeVerifier).toNSString()];
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

//
//  YoutubeAuth_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 11/12/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "YoutubeAuth.hpp"
#include "YoutubeError.hpp"

#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <Foundation/Foundation.h>
#import <soundhole/utils/HttpClient.hpp>
#import <soundhole/utils/ios/SHWebAuthNavigationController_iOS.h>
#import <soundhole/utils/ios/SHiOSUtils.h>

namespace sh {
	Promise<Optional<YoutubeSession>> YoutubeAuth::authenticate(AuthenticateOptions options) {
		return Promise<Optional<YoutubeSession>>([=](auto resolve, auto reject) {
			dispatch_async(dispatch_get_main_queue(), ^{
				String codeVerifier = String::random(64,
					"ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" "0123456789" "-._~");
				
				SHWebAuthNavigationController* authController = [[SHWebAuthNavigationController alloc] init];
				authController.handleNavigationAction = ^(SHWebAuthNavigationController* authNav, WKNavigationAction* action, void(^decisionHandler)(WKNavigationActionPolicy)) {
					// check if redirect URL matches
					NSURL* url = action.request.URL;
					if(!utils::checkURLMatch(options.redirectURL, String(url.absoluteString))) {
						decisionHandler(WKNavigationActionPolicyAllow);
						return;
					}
					// URL matches redirect URL
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
					YoutubeAuth::handleOAuthRedirect(params, options, codeVerifier).then([=](Optional<YoutubeSession> session) {
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
				authController.webViewController.webView.customUserAgent = @"Mozilla/5.0";
				NSURL* url = [NSURL URLWithString:options.getWebAuthenticationURL(codeVerifier).toNSString()];
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

//
//  BandcampAuth_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 11/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "BandcampAuth.hpp"
#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <soundhole/utils/ios/SHiOSUtils.h>
#import <soundhole/utils/ios/SHWebAuthNavigationController_iOS.h>
#import <soundhole/utils/objc/SHObjcUtils.h>
#import <WebKit/WebKit.h>

namespace sh {
	Promise<Optional<BandcampSession>> BandcampAuth::authenticate() {
		return Promise<Optional<BandcampSession>>([=](auto resolve, auto reject) {
			dispatch_async(dispatch_get_main_queue(), ^{
				SHWebAuthNavigationController* viewController = [[SHWebAuthNavigationController alloc] init];
				
				viewController.handleNavigationAction = ^(SHWebAuthNavigationController* viewController, WKNavigationAction* action, void(^decisionHandler)(WKNavigationActionPolicy)) {
					// fetch cookies to check for required cookies
					WKWebView* webView = viewController.webView;
					WKHTTPCookieStore* cookieStore = webView.configuration.websiteDataStore.httpCookieStore;
					[cookieStore getAllCookies:^(NSArray<NSHTTPCookie*>* cookies) {
						// look for client_id and identity cookies
						BOOL hasClientId = NO;
						BOOL hasIdentity = NO;
						for(NSHTTPCookie* cookie in cookies) {
							if([cookie.domain isEqualToString:@"bandcamp.com"] || [cookie.domain isEqualToString:@".bandcamp.com"]) {
								if([cookie.name isEqualToString:@"client_id"]) {
									hasClientId = YES;
								} else if([cookie.name isEqualToString:@"identity"]) {
									hasIdentity = YES;
								}
							}
						}
						if(!hasClientId || !hasIdentity) {
							decisionHandler(WKNavigationActionPolicyAllow);
						}
						// we have the required cookies, so we can move on
						decisionHandler(WKNavigationActionPolicyCancel);
						// build cookies array
						ArrayList<String> cookiesArray;
						cookiesArray.reserve((size_t)cookies.count);
						for(NSHTTPCookie* cookie in cookies) {
							cookiesArray.pushBack(String([SHObjcUtils stringFromCookie:cookie]));
						}
						// show loading overlay
						[viewController setLoadingOverlayVisible:YES animated:YES];
						// serialize a bandcamp session from the cookies
						createBandcampSessionFromCookies(cookiesArray).then([=](auto session) {
							dispatch_async(dispatch_get_main_queue(), ^{
								// hide loading overlay
								[viewController setLoadingOverlayVisible:NO animated:YES];
								// dismiss screen
								[viewController.presentingViewController dismissViewControllerAnimated:YES completion:^{
									// if we have a session, we were successful, otherwise we failed
									if(session) {
										resolve(session.value());
									} else {
										reject(std::runtime_error("Couldn't create bandcamp session from cookies"));
									}
								}];
							});
						}, reject);
					}];
				};
				
				viewController.onCancel = ^{
					resolve(std::nullopt);
				};
				
				[viewController loadViewIfNeeded];
				[viewController.webViewController loadViewIfNeeded];
				NSURL* url = [NSURL URLWithString:@"https://bandcamp.com/login"];
				NSURLRequest* request = [NSURLRequest requestWithURL:url];
				[viewController.webView loadRequest:request];
				
				// present auth controller
				UIViewController* topViewController = [SHiOSUtils topVisibleViewController];
				[topViewController presentViewController:viewController animated:YES completion:nil];
			});
		});
	}
}

#endif

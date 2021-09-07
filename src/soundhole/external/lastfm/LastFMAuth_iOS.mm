//
//  LastFMAuth_iOS.m
//  SoundHoleCore
//
//  Created by Luis Finke on 9/6/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "LastFMAuth.hpp"
#include <soundhole/utils/HttpClient.hpp>

#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <Foundation/Foundation.h>
#import <soundhole/utils/objc/SHObjcUtils.h>
#import <soundhole/utils/ios/SHiOSUtils.h>
#import <soundhole/utils/ios/SHWebAuthNavigationController_iOS.h>

namespace sh {
	bool LastFMAuth::isWebLoggedIn() const {
		NSHTTPCookieStorage* cookieStorage = NSHTTPCookieStorage.sharedHTTPCookieStorage;
		NSArray<NSHTTPCookie*>* cookies = [cookieStorage cookiesForURL:[NSURL URLWithString:_apiConfig.webLoginBaseURL().toNSString()]];
		auto webLoginHost = _apiConfig.webLoginBaseHost().toNSString();
		auto webLoginSubHost = [@"." stringByAppendingString:webLoginHost];
		auto sessCookieName = _apiConfig.webSessionCookieName.toNSString();
		for(NSHTTPCookie* cookie in cookies) {
			if(([cookie.domain isEqualToString:webLoginHost] || [cookie.domain isEqualToString:webLoginSubHost]) && [cookie.name isEqualToString:sessCookieName]) {
				return _apiConfig.checkSessionCookieLoggedIn == nullptr || _apiConfig.checkSessionCookieLoggedIn(String(cookie.value));
			}
		}
		return NO;
	}
	
	Promise<Optional<LastFMWebAuthResult>> LastFMAuth::authenticateWeb(Function<Promise<void>(LastFMWebAuthResult)> beforeDismiss) const {
		auto webLoginURL = _apiConfig.webLogin;
		return Promise<Optional<LastFMWebAuthResult>>([=](auto resolve, auto reject) {
			dispatch_async(dispatch_get_main_queue(), ^{
				SHWebAuthNavigationController* viewController = [[SHWebAuthNavigationController alloc] init];
				
				struct SharedData {
					String username;
					String password;
				};
				auto sharedData = fgl::new$<SharedData>(SharedData{});
				
				viewController.handleNavigationAction = ^(SHWebAuthNavigationController* viewController, WKNavigationAction* action, void(^decisionHandler)(WKNavigationActionPolicy)) {
					// check for username / password request
					if(action.request.URL != nil && [action.request.URL isEqual:[NSURL URLWithString:webLoginURL.toNSString()]]
					   && [action.request.HTTPMethod isEqualToString:@"POST"]) {
						auto httpBody = action.request.HTTPBody ? [[NSString alloc] initWithData:action.request.HTTPBody encoding:NSUTF8StringEncoding] : nil;
						if(httpBody != nil) {
							// store username / password
							auto params = utils::parseQueryString(String(httpBody));
							auto username = params.get("username_or_email", String());
							auto password = params.get("password", String());
							if(!username.empty()) {
								sharedData->username = username;
							}
							if(!password.empty()) {
								sharedData->password = password;
							}
						}
					}
					// fetch cookies to check for required cookies
					WKWebView* webView = viewController.webView;
					WKHTTPCookieStore* cookieStore = webView.configuration.websiteDataStore.httpCookieStore;
					[cookieStore getAllCookies:^(NSArray<NSHTTPCookie*>* cookies) {
						// look for client_id and identity cookies
						BOOL isAuthenticated = NO;
						auto webLoginHost = _apiConfig.webLoginHost().toNSString();
						auto webLoginBaseHost = _apiConfig.webLoginBaseHost().toNSString();
						auto webLoginSubHost = [@"." stringByAppendingString:webLoginBaseHost];
						auto sessCookieName = _apiConfig.webSessionCookieName.toNSString();
						for(NSHTTPCookie* cookie in cookies) {
							if([cookie.domain isEqualToString:webLoginBaseHost] || [cookie.domain isEqualToString:webLoginSubHost]) {
								if([cookie.name isEqualToString:sessCookieName]) {
									if(_apiConfig.checkSessionCookieLoggedIn == nullptr || _apiConfig.checkSessionCookieLoggedIn(String(cookie.value))) {
										isAuthenticated = YES;
										break;
									}
								}
							}
						}
						if(!isAuthenticated) {
							decisionHandler(WKNavigationActionPolicyAllow);
							return;
						}
						// we have the required cookies, so we can move on
						decisionHandler(WKNavigationActionPolicyCancel);
						// build cookies array
						ArrayList<String> cookiesArray;
						cookiesArray.reserve((size_t)cookies.count);
						for(NSHTTPCookie* cookie in cookies) {
							if([cookie.domain isEqualToString:webLoginBaseHost] || [cookie.domain isEqualToString:webLoginSubHost] || [cookie.domain isEqualToString:webLoginHost]) {
								cookiesArray.pushBack(String([SHObjcUtils stringFromCookie:cookie]));
							}
						}
						// show loading overlay
						[viewController setLoadingOverlayVisible:YES animated:YES];
						// define function to dismiss controller
						auto dismissAuthController = [=](void(^completion)(void)) {
							// hide loading overlay
							[viewController setLoadingOverlayVisible:NO animated:YES];
							// dismiss controller
							if(viewController.presentingViewController != nil) {
								[viewController.presentingViewController dismissViewControllerAnimated:YES completion:completion];
							} else {
								completion();
							}
						};
						// create web result
						auto webResult = LastFMWebAuthResult{
							.username = sharedData->username,
							.password = sharedData->password,
							.cookies = cookiesArray
						};
						// run before dismissal logic
						(beforeDismiss ? beforeDismiss(webResult) : Promise<void>::resolve()).then([=]() {
							// dismiss with result
							dismissAuthController(^{
								resolve(webResult);
							});
						}, [=](std::exception_ptr error) {
							// dismiss with error
							dismissAuthController(^{
								reject(error);
							});
						});
					}];
				};
				
				viewController.onCancel = ^{
					resolve(std::nullopt);
				};
				
				[viewController loadViewIfNeeded];
				[viewController.webViewController loadViewIfNeeded];
				viewController.webView.configuration.websiteDataStore = [WKWebsiteDataStore nonPersistentDataStore];
				NSURL* url = [NSURL URLWithString:_apiConfig.webLogin.toNSString()];
				NSURLRequest* request = [NSURLRequest requestWithURL:url];
				[viewController.webView loadRequest:request];
				
				// present auth controller
				UIViewController* topViewController = [SHiOSUtils topVisibleViewController];
				[topViewController presentViewController:viewController animated:YES completion:nil];
			});
		});
	}
	
	void LastFMAuth::applyWebAuthResult(LastFMWebAuthResult webResult) {
		// parse cookies
		NSURL* lastFMURL = [NSURL URLWithString:_apiConfig.webLogin.toNSString()];
		NSMutableArray<NSHTTPCookie*>* cookies = [NSMutableArray array];
		for(auto cookie : webResult.cookies) {
			NSArray<NSHTTPCookie*>* nsCookies = [NSHTTPCookie cookiesWithResponseHeaderFields:@{
				@"Set-Cookie": cookie.toNSString()
			} forURL:lastFMURL];
			[cookies addObjectsFromArray:nsCookies];
		}
		// store cookies in shared storage
		NSHTTPCookieStorage* cookieStorage = NSHTTPCookieStorage.sharedHTTPCookieStorage;
		[cookieStorage setCookies:cookies forURL:lastFMURL mainDocumentURL:nil];
	}
}

#endif

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
#import <soundhole/utils/ios/SHWebAuthNavigationController_iOS.h>
#import <soundhole/utils/ios/SHiOSUtils.h>
#import <soundhole/utils/objc/SHObjcUtils.h>

namespace sh {
	void SpotifyAuth_handleRedirectParams(NSDictionary* params, const SpotifyAuth::Options& options, NSString* xssState, void(^completion)(Optional<SpotifySession>,std::exception_ptr));

	Promise<Optional<SpotifySession>> SpotifyAuth::authenticate(Options options) {
		if(options.params.find("show_dialog") == options.params.end()) {
			options.params["show_dialog"] = "true";
		}
		return Promise<Optional<SpotifySession>>([=](auto resolve, auto reject) {
			dispatch_async(dispatch_get_main_queue(), ^{
				NSString* xssState = [NSUUID UUID].UUIDString;
				
				SHWebAuthNavigationController* authController = [[SHWebAuthNavigationController alloc] init];
				authController.onWebRedirect = ^BOOL(SHWebAuthNavigationController* authNav, WKWebView* webView, WKNavigationAction* action) {
					// check if redirect URL matches
					NSURL* url = action.request.URL;
					if(![SHObjcUtils checkIfURL:url matchesRedirectURL:[NSURL URLWithString:options.redirectURL.toNSString()]]) {
						return NO;
					}
					[authNav setLoadingOverlayVisible:YES animated:YES];
					NSDictionary* params = [SHObjcUtils parseOAuthQueryParams:url];
					SpotifyAuth_handleRedirectParams(params, options, xssState, ^(Optional<SpotifySession> session, std::exception_ptr error) {
						[authNav setLoadingOverlayVisible:NO animated:YES];
						auto finishLogin = [=]() {
							if(error) {
								reject(error);
							} else {
								resolve(session);
							}
						};
						if(authNav.presentingViewController != nil) {
							dispatch_async(dispatch_get_main_queue(), ^{
								[authNav.presentingViewController dismissViewControllerAnimated:YES completion:^{
									finishLogin();
								}];
							});
						} else {
							finishLogin();
						}
					});
					return YES;
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


	void SpotifyAuth_handleRedirectParams(NSDictionary* params, const SpotifyAuth::Options& options, NSString* xssState, void(^completion)(Optional<SpotifySession>,std::exception_ptr)) {
		NSString* state = params[@"state"];
		NSString* error = params[@"error"];
		if(error != nil) {
			if([error isEqualToString:@"access_denied"]) {
				// cancelled from webview
				completion(std::nullopt, nullptr);
			}
			else {
				// error
				completion(std::nullopt, std::make_exception_ptr(SpotifyError(SpotifyError::Code::OAUTH_REQUEST_FAILED, String(error))));
			}
		}
		else if(xssState != nil && (state == nil || ![xssState isEqualToString:state])) {
			// state mismatch
			completion(std::nullopt, std::make_exception_ptr(SpotifyError(SpotifyError::Code::OAUTH_STATE_MISMATCH, "State mismatch")));
		}
		else if(params[@"access_token"] != nil) {
			// access token
			NSString* accessToken = params[@"access_token"];
			NSString* expiresIn = params[@"expires_in"];
			NSInteger expireSeconds = [expiresIn integerValue];
			if(expireSeconds == 0) {
				if(completion != nil) {
					completion(std::nullopt, std::make_exception_ptr(SpotifyError(SpotifyError::Code::BAD_RESPONSE, "Access token expire time was 0")));
				}
				return;
			}
			NSString* scope = params[@"scope"];
			ArrayList<String> scopes;
			if(scope != nil) {
				NSArray* scopesArr = [scope componentsSeparatedByString:@" "];
				for(NSString* scope in scopesArr) {
					scopes.pushBack(String(scope));
				}
			} else {
				scopes = options.scopes;
			}
			NSString* refreshToken = params[@"refreshToken"];
			auto session = SpotifySession(
				String(accessToken),
				SpotifySession::getExpireTimeFromSeconds((int)expireSeconds),
				(refreshToken != nil) ? String(refreshToken) : "",
				scopes
			);
			completion(session, nullptr);
		}
		else if(params[@"code"] != nil) {
			// authentication code
			if(options.tokenSwapURL.empty()) {
				completion(std::nullopt, std::make_exception_ptr(SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "Missing tokenSwapURL")));
				return;
			}
			NSString* code = params[@"code"];
			SpotifyAuth::swapCodeForToken(String(code), options.tokenSwapURL).then([=](SpotifySession session) {
				if(session.getScopes().empty() && !options.scopes.empty()) {
					session = SpotifySession(
						session.getAccessToken(),
						session.getExpireTime(),
						session.getRefreshToken(),
						options.scopes);
				}
				dispatch_async(dispatch_get_main_queue(), ^{
					completion(session, nullptr);
				});
			}).except([=](std::exception_ptr error) {
				dispatch_async(dispatch_get_main_queue(), ^{
					completion(std::nullopt, error);
				});
			});
		}
		else {
			completion(std::nullopt, std::make_exception_ptr(SpotifyError(SpotifyError::Code::BAD_RESPONSE, "Missing expected parameters in redirect URL")));
		}
	}
}

#endif

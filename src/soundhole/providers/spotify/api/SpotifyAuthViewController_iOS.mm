//
//  SpotifyAuthViewController_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 9/27/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SpotifyAuthViewController_iOS.hpp"
#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <soundhole/utils/ios/SHWebViewController_iOS.hpp>
#import <soundhole/utils/ios/SHProgressView_iOS.hpp>

using namespace sh;

@interface SpotifyAuthViewController() {
	SpotifyAuth::Options _options;
	NSString* _xssState;
	
	SHWebViewController* _webController;
	SHProgressView* _progressView;
}
-(void)didSelectCancelButton;
@end

@implementation SpotifyAuthViewController

+(UIViewController*)topVisibleViewController {
	UIViewController* topController = [UIApplication sharedApplication].keyWindow.rootViewController;
	while(topController.presentedViewController != nil) {
		topController = topController.presentedViewController;
	}
	return topController;
}

-(id)initWithOptions:(SpotifyAuth::Options)options {
	SHWebViewController* rootController = [[SHWebViewController alloc] init];
	if(self = [super initWithRootViewController:rootController]) {
		_webController = rootController;
		_progressView = [[SHProgressView alloc] init];
		
		_options = options;
		_xssState = [NSUUID UUID].UUIDString;
		
		self.navigationBar.barTintColor = [UIColor blackColor];
		self.navigationBar.tintColor = [UIColor whiteColor];
		self.navigationBar.titleTextAttributes = @{NSForegroundColorAttributeName : [UIColor whiteColor]};
		self.view.backgroundColor = [UIColor whiteColor];
		self.modalPresentationStyle = UIModalPresentationFormSheet;
		
		_webController.webView.navigationDelegate = self;
		//_webController.title = @"Log into Spotify";
		_webController.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:self action:@selector(didSelectCancelButton)];
		
		NSURL* url = [NSURL URLWithString:_options.getWebAuthenticationURL(String(_xssState)).toNSString()];
		NSURLRequest* request = [NSURLRequest requestWithURL:url];
		[_webController.webView loadRequest:request];
	}
	return self;
}

-(void)viewDidLoad {
	[super viewDidLoad];
	
	self.presentationController.delegate = self;
}

-(UIStatusBarStyle)preferredStatusBarStyle {
	return UIStatusBarStyleLightContent;
}

-(void)didSelectCancelButton {
	if(_completion != nil) {
		_completion(std::nullopt, nullptr);
	}
}

-(void)presentationControllerDidDismiss:(UIPresentationController*)presentationController {
	if(_completion != nil) {
		_completion(std::nullopt, nullptr);
	}
}


#pragma mark - auth methods

+(NSDictionary*)decodeQueryString:(NSString*)queryString {
	NSArray<NSString*>* parts = [queryString componentsSeparatedByString:@"&"];
	NSMutableDictionary* params = [NSMutableDictionary dictionary];
	for(NSString* part in parts) {
		NSString* escapedPart = [part stringByReplacingOccurrencesOfString:@"+" withString:@"%20"];
		NSArray<NSString*>* expressionParts = [escapedPart componentsSeparatedByString:@"="];
		if(expressionParts.count != 2) {
			continue;
		}
		NSString* key = expressionParts[0].stringByRemovingPercentEncoding;
		NSString* value = expressionParts[1].stringByRemovingPercentEncoding;
		params[key] = value;
	}
	return params;
}

+(NSDictionary*)parseOAuthQueryParams:(NSURL*)url {
	if(url == nil) {
		return [NSDictionary dictionary];
	}
	NSDictionary* queryParams = [self decodeQueryString:url.query];
	if(queryParams != nil && queryParams.count > 0) {
		return queryParams;
	}
	NSDictionary* fragmentParams = [self decodeQueryString:url.fragment];
	if(fragmentParams != nil && fragmentParams.count > 0) {
		return fragmentParams;
	}
	return [NSDictionary dictionary];
}

-(BOOL)canHandleRedirectURL:(NSURL*)url {
	if(_options.redirectURL.empty()) {
		return NO;
	}
	NSURL* redirectURL = [NSURL URLWithString:_options.redirectURL.toNSString()];
	if(![url.absoluteString hasPrefix:redirectURL.absoluteString]) {
		return NO;
	}
	
	NSString* path = redirectURL.path;
	if(path == nil || [path isEqualToString:@"/"]) {
		path = @"";
	}
	NSString* cmpPath = url.path;
	if(cmpPath == nil || [cmpPath isEqualToString:@"/"]) {
		cmpPath = @"";
	}
	if(![path isEqualToString:cmpPath]) {
		return NO;
	}
	return YES;
}

-(void)handleRedirectURL:(NSURL*)url completion:(SpotifyAuthViewControllerCompletion)completion {
	NSDictionary* params = [self.class parseOAuthQueryParams:url];
	NSString* state = params[@"state"];
	NSString* error = params[@"error"];
	if(error != nil) {
		if([error isEqualToString:@"access_denied"]) {
			// cancelled from webview
			if(completion != nil) {
				completion(std::nullopt, nullptr);
			}
		}
		else {
			// error
			if(completion != nil) {
				completion(std::nullopt, std::make_exception_ptr(SpotifyError(SpotifyError::Code::OAUTH_REQUEST_FAILED, String(error))));
			}
		}
	}
	else if(_xssState != nil && (state == nil || ![_xssState isEqualToString:state])) {
		// state mismatch
		if(completion != nil) {
			completion(std::nullopt, std::make_exception_ptr(SpotifyError(SpotifyError::Code::OAUTH_STATE_MISMATCH, "State mismatch")));
		}
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
			scopes = _options.scopes;
		}
		NSString* refreshToken = params[@"refreshToken"];
		auto session = SpotifySession(
			String(accessToken),
			SpotifySession::getExpireTimeFromSeconds((int)expireSeconds),
			(refreshToken != nil) ? String(refreshToken) : "",
			scopes
		);
		if(completion != nil) {
			completion(session, nullptr);
		}
	}
	else if(params[@"code"] != nil) {
		// authentication code
		if(_options.tokenSwapURL.empty()) {
			if(completion != nil) {
				completion(std::nullopt, std::make_exception_ptr(SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "Missing tokenSwapURL")));
			}
			return;
		}
		NSString* code = params[@"code"];
		SpotifyAuth::swapCodeForToken(String(code), _options.tokenSwapURL).then([=](SpotifySession session) {
			if(session.getScopes().empty() && !_options.scopes.empty()) {
				session = SpotifySession(
					session.getAccessToken(),
					session.getExpireTime(),
					session.getRefreshToken(),
					_options.scopes);
			}
			if(completion != nil) {
				dispatch_async(dispatch_get_main_queue(), ^{
					completion(session, nullptr);
				});
			}
		}).except([=](std::exception_ptr error) {
			if(completion != nil) {
				dispatch_async(dispatch_get_main_queue(), ^{
					completion(std::nullopt, error);
				});
			}
		});
	}
	else {
		if(completion != nil) {
			completion(std::nullopt, std::make_exception_ptr(SpotifyError(SpotifyError::Code::BAD_RESPONSE, "Missing expected parameters in redirect URL")));
		}
	}
}


#pragma mark - UIWebViewDelegate

-(void)webView:(WKWebView*)webView decidePolicyForNavigationAction:(WKNavigationAction*)navigationAction decisionHandler:(void(^)(WKNavigationActionPolicy))decisionHandler {
	if([self canHandleRedirectURL:navigationAction.request.URL]) {
		[_progressView showInView:self.view animated:YES completion:nil];
		[self handleRedirectURL:navigationAction.request.URL completion:^(sh::Optional<sh::SpotifySession> session, std::exception_ptr error) {
			[self->_progressView dismissAnimated:YES completion:nil];
			if(self.presentingViewController == nil) {
				return;
			}
			self->_completion(session, error);
		}];
		decisionHandler(WKNavigationActionPolicyCancel);
	}
	else {
		decisionHandler(WKNavigationActionPolicyAllow);
	}
}

@end

#endif

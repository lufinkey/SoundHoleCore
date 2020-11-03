//
//  BandcampAuthViewController_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 11/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "BandcampAuthViewController_iOS.h"
#include <soundhole/utils/HttpClient.hpp>
#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <soundhole/utils/ios/SHProgressView_iOS.h>

@interface BandcampAuthViewController() {
	SHProgressView* _loadingOverlayView;
	BOOL _hasLoadedInitialURL;
}
@end

@implementation BandcampAuthViewController

-(id)init {
	WKWebViewConfiguration* webViewConfig = [[WKWebViewConfiguration alloc] init];
	webViewConfig.websiteDataStore = [WKWebsiteDataStore nonPersistentDataStore];
	
	SHWebViewController* webViewController = [[SHWebViewController alloc] initWithConfiguration:webViewConfig];
	if(self = [super initWithRootViewController:webViewController]) {
		self.modalPresentationStyle = UIModalPresentationFormSheet;
		_statusBarStyle = UIStatusBarStyleDefault;
		_webViewController = webViewController;
		[_webViewController loadViewIfNeeded];
		
		_webViewController.webView.navigationDelegate = self;
		_webViewController.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:self action:@selector(didPressCancelButton)];
	}
	return self;
}

-(void)viewDidLoad {
	[super viewDidLoad];
	self.presentationController.delegate = self;
}

-(void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	if(!_hasLoadedInitialURL) {
		_hasLoadedInitialURL = YES;
		NSURLRequest* request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"https://bandcamp.com/login"]];
		[self.webViewController.webView loadRequest:request];
	}
}

-(UIStatusBarStyle)preferredStatusBarStyle {
	return _statusBarStyle;
}

-(void)setStatusBarStyle:(UIStatusBarStyle)statusBarStyle {
	if(_statusBarStyle != statusBarStyle) {
		_statusBarStyle = statusBarStyle;
		[self setNeedsStatusBarAppearanceUpdate];
	}
}


-(void)setLoadingOverlayVisible:(BOOL)loadingOverlayVisible animated:(BOOL)animated {
	if((_loadingOverlayVisible && !loadingOverlayVisible) || (!_loadingOverlayVisible || loadingOverlayVisible)) {
		return;
	}
	_loadingOverlayVisible = loadingOverlayVisible;
	if(_loadingOverlayView == nil) {
		_loadingOverlayView = [[SHProgressView alloc] init];
	}
	if(_loadingOverlayVisible) {
		[_loadingOverlayView showInView:self.view animated:animated completion:nil];
	} else {
		[_loadingOverlayView dismissAnimated:animated completion:nil];
	}
}

-(void)setLoadingOverlayVisible:(BOOL)loadingOverlayVisible {
	[self setLoadingOverlayVisible:loadingOverlayVisible animated:NO];
}



-(void)didPressCancelButton {
	[self.presentingViewController dismissViewControllerAnimated:YES completion:^{
		if(self->_onCancel != nil) {
			self->_onCancel();
		}
	}];
}

-(BOOL)presentationControllerShouldDismiss:(UIPresentationController*)presentationController {
	return !_loadingOverlayVisible;
}

-(void)presentationControllerDidDismiss:(UIPresentationController*)presentationController {
	if(_onCancel != nil) {
		_onCancel();
	}
}



#pragma mark - WKWebViewDelegate

-(void)webView:(WKWebView*)webView decidePolicyForNavigationAction:(WKNavigationAction*)action decisionHandler:(void(^)(WKNavigationActionPolicy))decisionHandler {
	WKHTTPCookieStore* cookieStore = webView.configuration.websiteDataStore.httpCookieStore;
	[cookieStore getAllCookies:^(NSArray<NSHTTPCookie*>* cookiesArr) {
		std::map<fgl::String,fgl::String> cookies;
		for(NSHTTPCookie* cookie in cookiesArr) {
			if([cookie.domain isEqualToString:@"bandcamp.com"] || [cookie.domain isEqualToString:@".bandcamp.com"]) {
				cookies[fgl::String(cookie.name)] = fgl::String(cookie.value);
			}
		}
		auto session = sh::BandcampSession::fromCookies(cookies);
		if(session) {
			decisionHandler(WKNavigationActionPolicyCancel);
			self->_onSuccess(self,session.value());
		} else {
			decisionHandler(WKNavigationActionPolicyAllow);
		}
	}];
}

@end

#endif

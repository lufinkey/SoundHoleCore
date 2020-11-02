//
//  SHWebAuthNavigationController_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 9/13/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "SHWebAuthNavigationController_iOS.h"
#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <soundhole/utils/ios/SHProgressView_iOS.h>

@interface SHWebAuthNavigationController() {
	SHProgressView* _loadingOverlayView;
}
@end

@implementation SHWebAuthNavigationController

-(id)init {
	SHWebViewController* webViewController = [[SHWebViewController alloc] init];
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

-(void)webView:(WKWebView*)webView decidePolicyForNavigationAction:(WKNavigationAction*)navigationAction decisionHandler:(void(^)(WKNavigationActionPolicy))decisionHandler {
	if(self.onWebRedirect != nil && self.onWebRedirect(self,webView,navigationAction)) {
		decisionHandler(WKNavigationActionPolicyCancel);
	}
	else {
		decisionHandler(WKNavigationActionPolicyAllow);
	}
}

@end

#endif

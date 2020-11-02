//
//  SpotifyWebViewController_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 9/27/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SHWebViewController_iOS.h"
#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)

@implementation SHWebViewController

-(id)init {
	if(self = [super init]) {
		//
	}
	return self;
}

-(void)viewDidLoad {
	[super viewDidLoad];
	
	_webView = [[WKWebView alloc] init];
	[self.view addSubview:_webView];
}

-(void)viewWillLayoutSubviews {
	[super viewWillLayoutSubviews];
	CGSize size = self.view.bounds.size;
	
	_webView.frame = CGRectMake(0,0,size.width, size.height);
}

@end

#endif

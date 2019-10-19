//
//  SHProgressView.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 9/27/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SHProgressView_iOS.hpp"
#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <QuartzCore/QuartzCore.h>

@interface SHProgressView() {
	UIView* _hudView;
}
@end

@implementation SHProgressView

-(id)initWithFrame:(CGRect)frame {
	if(self = [super initWithFrame:frame]) {
		self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
		
		CGSize hudSize = CGSizeMake(100, 100);
		
		_hudView = [[UIView alloc] initWithFrame:CGRectMake(0,0,hudSize.width,hudSize.height)];
		_hudView.backgroundColor = [UIColor colorWithWhite:0 alpha:0.6];
		_hudView.layer.cornerRadius = 10;
		
		_activityIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
		_activityIndicator.center = CGPointMake(hudSize.width/2, hudSize.height/2);
		
		[_hudView addSubview:_activityIndicator];
		[self addSubview:_hudView];
	}
	return self;
}

-(void)layoutSubviews {
	[super layoutSubviews];
	CGSize size = self.bounds.size;
	
	_hudView.center = CGPointMake(size.width/2, size.height/2);
}

-(void)willMoveToSuperview:(UIView*)newSuperview {
	[super willMoveToSuperview:newSuperview];
	if(newSuperview != nil) {
		[_activityIndicator startAnimating];
	}
	else {
		[_activityIndicator stopAnimating];
	}
}

-(void)showInView:(UIView*)view animated:(BOOL)animated completion:(void(^)(void))completion {
	CGSize viewSize = view.bounds.size;
	self.frame = CGRectMake(0, 0, viewSize.width, viewSize.height);
	[self setNeedsLayout];
	
	if(animated) {
		_hudView.alpha = 0;
		_hudView.transform = CGAffineTransformMakeScale(1.4, 1.4);
		[view addSubview:self];
		[UIView animateWithDuration:0.25 animations:^{
			self->_hudView.alpha = 1;
			self->_hudView.transform = CGAffineTransformIdentity;
		} completion:^(BOOL finished) {
			if(completion != nil) {
				completion();
			}
		}];
	}
	else {
		[view addSubview:self];
		if(completion != nil) {
			completion();
		}
	}
}

-(void)dismissAnimated:(BOOL)animated completion:(void(^)(void))completion {
	if(animated) {
		[UIView animateWithDuration:0.25 animations:^{
			self.alpha = 0;
		} completion:^(BOOL finished) {
			[self removeFromSuperview];
			self.alpha = 1;
			if(completion!=nil) {
				completion();
			}
		}];
	}
	else {
		[self removeFromSuperview];
		if(completion!=nil) {
			completion();
		}
	}
}

@end

#endif

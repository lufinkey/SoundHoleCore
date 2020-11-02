//
//  SHiOSUtils.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 11/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#import "SHiOSUtils.h"

@implementation SHiOSUtils

+(UIViewController*)topVisibleViewController {
	UIViewController* topController = UIApplication.sharedApplication.keyWindow.rootViewController;
	while(topController.presentedViewController != nil) {
		topController = topController.presentedViewController;
	}
	return topController;
}

@end

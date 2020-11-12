//
//  Utils.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 11/12/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "Utils.hpp"

#if defined(__OBJC__) && (defined(TARGETPLATFORM_IOS) || defined(TARGETPLATFORM_MAC))
#import <Foundation/Foundation.h>

namespace sh::utils {
	String getTmpDirectoryPath() {
		return String(NSFileManager.defaultManager.temporaryDirectory.absoluteString);
	}
	
	String getCacheDirectoryPath() {
		NSArray<NSString*>* paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
		if(paths.count == 0) {
			return getTmpDirectoryPath();
		}
		return String(paths.firstObject);
	}
}

#endif

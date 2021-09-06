//
//  Utils_apple.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 11/12/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "Utils.hpp"
#import "objc/SHObjcUtils.h"

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



	void setPreferenceValue(String key, Json json) {
		NSUserDefaults* userDefaults = NSUserDefaults.standardUserDefaults;
		NSObject* obj = nsObjectFromJson(json);
		if(obj == nil) {
			[userDefaults removeObjectForKey:key.toNSString()];
		} else {
			[userDefaults setObject:obj forKey:key.toNSString()];
		}
	}

	Json getPreferenceValue(String key) {
		NSObject* obj = [NSUserDefaults.standardUserDefaults objectForKey:key.toNSString()];
		return jsonFromNSObject(obj);
	}
}

#endif

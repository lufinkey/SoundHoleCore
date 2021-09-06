//
//  SecureStore_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 9/5/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "SecureStore.hpp"

#if defined(__OBJC__) && (defined(TARGETPLATFORM_IOS) || defined(TARGETPLATFORM_MAC))
#import "objc/SHKeychainUtils_objc.h"

namespace sh {
	void SecureStore::setSecureData(const String& key, const Data& data) {
		[SHKeychainUtils setGenericPasswordData:data.toNSData() forAccountKey:key.toNSString()];
	}

	Data SecureStore::getSecureData(const String& key) {
		return [SHKeychainUtils genericPasswordDataForAccountKey:key.toNSString()];
	}
	
	void SecureStore::deleteSecureData(const String& key) {
		[SHKeychainUtils deleteGenericPasswordDataForAccountKey:key.toNSString()];
	}
}

#endif

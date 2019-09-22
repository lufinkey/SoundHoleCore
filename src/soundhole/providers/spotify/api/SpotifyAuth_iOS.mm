//
//  SpotifyAuth_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 9/22/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SpotifyAuth.hpp"

#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <Foundation/Foundation.h>

namespace sh {
	void SpotifyAuth::load() {
		if(options.sessionPersistKey.size() == 0) {
			return;
		}
		session = SpotifySession::fromNSUserDefaults(options.sessionPersistKey, NSUserDefaults.standardUserDefaults);
	}
	
	void SpotifyAuth::save() {
		if(options.sessionPersistKey.size() == 0) {
			return;
		}
		SpotifySession::writeToNSUserDefaults(session, options.sessionPersistKey, NSUserDefaults.standardUserDefaults);
	}
}

#endif

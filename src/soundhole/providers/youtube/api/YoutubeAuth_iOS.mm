//
//  YoutubeAuth_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 11/12/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "YoutubeAuth.hpp"
#include "YoutubeError.hpp"

#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <Foundation/Foundation.h>
#import <soundhole/utils/ios/SHWebAuthNavigationController_iOS.h>
#import <soundhole/utils/ios/SHiOSUtils.h>
#import <soundhole/utils/objc/SHObjcUtils.h>

namespace sh {
	Promise<Optional<YoutubeSession>> YoutubeAuth::authenticate(LoginOptions options) {
		// TODO implement youtube authentication screen
		return Promise<Optional<YoutubeSession>>::reject(std::logic_error("not implemented"));
	}
}

#endif

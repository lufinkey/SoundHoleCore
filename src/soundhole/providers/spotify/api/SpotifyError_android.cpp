//
//  SpotifyError_android.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/6/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#ifdef __ANDROID__

#include <jni.h>
#include "SpotifyError.hpp"
#include <soundhole/utils/android/AndroidUtils.hpp>
#include <tuple>
#include <utility>

#ifdef TARGETPLATFORM_ANDROID

namespace sh {
	SpotifyError::SpotifyError(JNIEnv* env, jobject error) {
		if(env->IsInstanceOf(error, android::SpotifyError::javaClass)) {
			int nativeCode = (int)android::SpotifyError::getNativeCode(env, error);
			try {
				auto entry = android::SpotifyError::enumMap[nativeCode];
				code = std::get<0>(entry);
				message = std::get<1>(entry);
			} catch(std::out_of_range&) {
				code = Code::SDK_FAILED;
				message = "An unidentifiable error occured";
			}
		}
	}
}

#endif
#endif

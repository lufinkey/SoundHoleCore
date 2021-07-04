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
#include <soundhole/jnicpp/android/spotify/SpotifyError_jni.hpp>
#include <tuple>
#include <utility>

namespace sh {
	namespace jni {
		using namespace android::spotify;
	}

	SpotifyError::SpotifyError(JNIEnv* env, jobject error) {
		if(env->IsInstanceOf(error, jni::SpotifyError::javaClass(env))) {
			auto errorObj = jni::SpotifyError(error);
			auto nativeCode = errorObj.getNativeCode(env);
			try {
				auto entry = jni::SpotifyError::enumMap[nativeCode];
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

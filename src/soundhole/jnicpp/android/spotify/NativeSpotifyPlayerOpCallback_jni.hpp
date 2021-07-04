
#pragma once

#include <soundhole/jnicpp/common.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	struct NativeSpotifyPlayerOpCallback: JNIObject {
		using JNIObject::JNIObject;
		static void init(JNIEnv* env);
		static FGL_JNI_DECL_JCLASS
		static FGL_JNI_DECL_JCONSTRUCTOR()

		static NativeSpotifyPlayerOpCallback newObject(JNIEnv* env, Function<void(JNIEnv*)> onResolve, Function<void(JNIEnv*,jobject)> onReject);
	};
}
#endif

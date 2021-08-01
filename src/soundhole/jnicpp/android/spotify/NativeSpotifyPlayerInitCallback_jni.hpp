
#pragma once

#include <soundhole/jnicpp/jnicpp_common.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	struct SpotifyPlayer;
	struct SpotifyError;

	struct NativeSpotifyPlayerInitCallback: JNIObject {
		using JNIObject::JNIObject;
		static void init(JNIEnv* env);
		static FGL_JNI_DECL_JCLASS
		static FGL_JNI_DECL_JCONSTRUCTOR()

		static NativeSpotifyPlayerInitCallback newObject(JNIEnv* env, Function<void(JNIEnv*,SpotifyPlayer)> onResolve, Function<void(JNIEnv*,jobject)> onReject);
	};
}
#endif

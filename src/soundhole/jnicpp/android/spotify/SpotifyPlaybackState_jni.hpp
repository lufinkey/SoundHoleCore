
#pragma once

#include <soundhole/jnicpp/common.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	struct SpotifyPlaybackState: JNIObject {
		using JNIObject::JNIObject;
		static void init(JNIEnv* env);
		static FGL_JNI_DECL_JCLASS
		static FGL_JNI_DECL_JFIELD(isPlaying)
		static FGL_JNI_DECL_JFIELD(isRepeating)
		static FGL_JNI_DECL_JFIELD(isShuffling)
		static FGL_JNI_DECL_JFIELD(isActiveDevice)
		static FGL_JNI_DECL_JFIELD(positionMs)

		jboolean isPlaying(JNIEnv* env);
		jboolean isRepeating(JNIEnv* env);
		jboolean isShuffling(JNIEnv* env);
		jboolean isActiveDevice(JNIEnv* env);
		jlong getPositionMs(JNIEnv* env);
	};
}
#endif

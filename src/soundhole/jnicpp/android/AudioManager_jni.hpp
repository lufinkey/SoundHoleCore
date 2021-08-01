
#pragma once

#include <soundhole/jnicpp/jnicpp_common.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android {
	struct AudioManager: JNIObject {
		using JNIObject::JNIObject;
		static void init(JNIEnv* env);
		static FGL_JNI_DECL_JCLASS
		static FGL_JNI_DECL_STATIC_JMETHOD(STREAM_MUSIC)

		static jint STREAM_MUSIC(JNIEnv* env);
	};
}
#endif


#pragma once

#include <soundhole/jnicpp/common.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android {
	struct Utils: JNIObject {
		static void init(JNIEnv* env);
		static FGL_JNI_DECL_JCLASS
		static FGL_JNI_DECL_STATIC_JFIELD(appContext)
		static FGL_JNI_DECL_STATIC_JMETHOD(runOnMainThread)

		static jobject getAppContext(JNIEnv* env);
		static void runOnMainThread(JNIEnv* env, Function<void(JNIEnv*)> func);
	};
}
#endif

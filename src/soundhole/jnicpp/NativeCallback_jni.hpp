
#pragma once

#include <soundhole/jnicpp/jnicpp_common.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni {
	struct NativeCallback: JNIObject {
		using JNIObject::JNIObject;
		static void init(JNIEnv* env);
		static FGL_JNI_DECL_JCLASS
		static FGL_JNI_DECL_JCONSTRUCTOR()

		static NativeCallback newObject(JNIEnv* env, Function<void(JNIEnv*,jobject)> onResolve, Function<void(JNIEnv*,jobject)> onReject);
	};
}
#endif


#pragma once

#include <soundhole/jnicpp/common.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni {
	struct NativeFunction: JNIObject {
		using JNIObject::JNIObject;
		using Callback = Function<void(JNIEnv*,std::vector<jobject>)>;
		static void init(JNIEnv* env);
		static FGL_JNI_DECL_JCLASS
		static FGL_JNI_DECL_JCONSTRUCTOR()

		static NativeFunction newObject(JNIEnv* env, Callback func);
	};
}
#endif

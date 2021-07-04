
#include "NativeFunction_jni.hpp"

#ifdef SOUNDHOLE_JNI_ENABLED

namespace sh::jni {
	FGL_JNI_DEF_JCLASS(NativeFunction, "com/lufinkey/soundholecore/NativeFunction")
	FGL_JNI_DEF_JCONSTRUCTOR(NativeFunction, , "(J)V")
	void NativeFunction::init(JNIEnv *env) {
		javaClass(env);
		methodID_constructor(env);
	}

	NativeFunction NativeFunction::newObject(JNIEnv* env, Callback func) {
		auto funcPtr = new Callback(func);
		return NativeFunction(env->NewObject(javaClass(env), methodID_constructor(env), (jlong)funcPtr));
	}
}

extern "C" JNIEXPORT void JNICALL
Java_com_lufinkey_soundholecore_NativeFunction_callFunction(JNIEnv* env, jobject, jlong funcPtr, jobjectArray argsArray) {
	sh::jni::NativeFunction::Callback func = *((sh::jni::NativeFunction::Callback*)funcPtr);
	std::vector<jobject> args;
	jsize argsSize = env->GetArrayLength(argsArray);
	args.reserve((size_t)argsSize);
	for(jsize i=0; i<argsSize; i++) {
		args.push_back(env->GetObjectArrayElement(argsArray, i));
	}
	func(env,args);
}

extern "C" JNIEXPORT void JNICALL
Java_com_lufinkey_soundholecore_NativeFunction_destroyFunction(JNIEnv* env, jobject, jlong funcPtr) {
	delete ((sh::jni::NativeFunction::Callback*)funcPtr);
}

#endif

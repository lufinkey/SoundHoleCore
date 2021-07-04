
#include <soundhole/jnicpp/NativeCallback_jni.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni {
	FGL_JNI_DEF_JCLASS(NativeCallback, "com/lufinkey/soundholecore/NativeCallback")
	FGL_JNI_DEF_JCONSTRUCTOR(NativeCallback, , "(JJ)V")
	void NativeCallback::init(JNIEnv* env) {
		javaClass(env);
		methodID_constructor(env);
	}

	NativeCallback NativeCallback::newObject(JNIEnv* env, Function<void(JNIEnv*,jobject)> onResolve, Function<void(JNIEnv*,jobject)> onReject) {
		return NativeCallback(env->NewObject(javaClass(env), methodID_constructor(env),
			(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onResolve(env, args[0]);
			})),
			(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onReject(env, args[0]);
			}))));
	}
}
#endif

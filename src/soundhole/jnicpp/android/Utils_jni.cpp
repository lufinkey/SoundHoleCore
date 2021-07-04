
#include <soundhole/jnicpp/android/Utils_jni.hpp>
#include <soundhole/jnicpp/NativeFunction_jni.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android {
	FGL_JNI_DEF_JCLASS(Utils, "com/lufinkey/soundholecore/android/Utils")
	FGL_JNI_DEF_STATIC_JFIELD(Utils, appContext, "appContext", "Landroid/content/Context;")
	FGL_JNI_DEF_STATIC_JMETHOD(Utils, runOnMainThread, "runOnMainThread", "(Lcom/lufinkey/soundholecore/NativeFunction;)V")
	void Utils::init(JNIEnv* env) {
		javaClass(env);
		fieldID_static_appContext(env);
		methodID_static_runOnMainThread(env);
	}

	jobject Utils::getAppContext(JNIEnv* env) {
		return env->GetStaticObjectField(javaClass(env), fieldID_static_appContext(env));
	}

	void Utils::runOnMainThread(JNIEnv *env, Function<void(JNIEnv*)> func) {
		env->CallStaticVoidMethod(javaClass(env), methodID_static_runOnMainThread(env),
			NativeFunction::newObject(env, [=](auto env, auto args) {
				func(env);
			}));
	}
}
#endif

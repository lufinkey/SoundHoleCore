
#include <soundhole/jnicpp/android/spotify/NativeSpotifyPlayerOpCallback_jni.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	FGL_JNI_DEF_JCLASS(NativeSpotifyPlayerOpCallback, "com/lufinkey/soundholecore/android/spotify/NativeSpotifyPlayerOpCallback")
	FGL_JNI_DEF_JCONSTRUCTOR(NativeSpotifyPlayerOpCallback, , "(JJ)V")
	void NativeSpotifyPlayerOpCallback::init(JNIEnv* env) {
		javaClass(env);
		methodID_constructor(env);
	}

	NativeSpotifyPlayerOpCallback NativeSpotifyPlayerOpCallback::newObject(JNIEnv* env, Function<void(JNIEnv*)> onResolve, Function<void(JNIEnv*,jobject)> onReject) {
		return NativeSpotifyPlayerOpCallback(env->NewObject(javaClass(env), methodID_constructor(env),
			(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onResolve(env);
			})),
			(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onReject(env, args[0]);
			}))));
	}
}
#endif

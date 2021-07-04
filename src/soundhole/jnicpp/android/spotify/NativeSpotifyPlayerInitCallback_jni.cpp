
#include <soundhole/jnicpp/android/spotify/NativeSpotifyPlayerInitCallback_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyPlayer_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyError_jni.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	FGL_JNI_DEF_JCLASS(NativeSpotifyPlayerInitCallback, "com/lufinkey/soundholecore/android/spotify/NativeSpotifyPlayerInitCallback")
	FGL_JNI_DEF_JCONSTRUCTOR(NativeSpotifyPlayerInitCallback, , "(JJ)V")
	void NativeSpotifyPlayerInitCallback::init(JNIEnv* env) {
		javaClass(env);
		methodID_constructor(env);
	}

	NativeSpotifyPlayerInitCallback NativeSpotifyPlayerInitCallback::newObject(JNIEnv* env, Function<void(JNIEnv*,SpotifyPlayer)> onResolve, Function<void(JNIEnv*,jobject)> onReject) {
		return NativeSpotifyPlayerInitCallback(env->NewObject(javaClass(env), methodID_constructor(env),
			(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onResolve(env, jni::android::spotify::SpotifyPlayer(args[0]));
			})),
			(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onReject(env, args[0]);
			}))));
	}
}
#endif

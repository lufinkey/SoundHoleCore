
#include <soundhole/jnicpp/android/spotify/SpotifyPlayerEventHandler_jni.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	FGL_JNI_DEF_JCLASS(SpotifyPlayerEventHandler, "com/lufinkey/soundholecore/android/spotify/SpotifyPlayerEventHandler")
	FGL_JNI_DEF_JCONSTRUCTOR(SpotifyPlayerEventHandler, , "(Lcom/spotify/sdk/android/player/SpotifyPlayer;JJJJJJJJJ)V")
	FGL_JNI_DEF_JMETHOD(SpotifyPlayerEventHandler, destroy, "destroy", "()V")
	void SpotifyPlayerEventHandler::init(JNIEnv* env) {
		javaClass(env);
		methodID_constructor(env);
		methodID_destroy(env);
	}

	SpotifyPlayerEventHandler SpotifyPlayerEventHandler::newObject(JNIEnv* env, ConstructorParams params) {
		auto onLoggedIn = params.onLoggedIn;
		auto onLoggedOut = params.onLoggedOut;
		auto onLoginFailed = params.onLoginFailed;
		auto onTemporaryError = params.onTemporaryError;
		auto onConnectionMessage = params.onConnectionMessage;
		auto onDisconnect = params.onDisconnect;
		auto onReconnect = params.onReconnect;
		auto onPlaybackEvent = params.onPlaybackEvent;
		auto onPlaybackError = params.onPlaybackError;
		return SpotifyPlayerEventHandler(env->NewObject(
			javaClass(env), methodID_constructor(env),
			params.player,
			(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onLoggedIn(env);
			})),
			(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onLoggedOut(env);
			})),
			(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onLoginFailed(env, args[0]);
			})),
			(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onTemporaryError(env);
			})),
			(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onConnectionMessage(env, (jstring)args[0]);
			})),
			(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onDisconnect(env);
			})),
			(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onReconnect(env);
			})),
			(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onPlaybackEvent(env, args[0]);
			})),
			(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onPlaybackError(env, args[0]);
			}))
		));
	}

	void SpotifyPlayerEventHandler::destroy(JNIEnv* env) {
		env->CallVoidMethod(value, methodID_destroy(env));
	}
}
#endif


#include <soundhole/jnicpp/android/spotify/SpotifyUtils_jni.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	FGL_JNI_DEF_JCLASS(SpotifyUtils, "com/lufinkey/soundholecore/android/spotify/SpotifyUtils")
	FGL_JNI_DEF_JCONSTRUCTOR(SpotifyUtils, , "()V")
	FGL_JNI_DEF_JMETHOD(SpotifyUtils, getPlayer, "getPlayer", "(Ljava/lang/String;Ljava/lang/String;Lcom/lufinkey/soundholecore/android/spotify/NativeSpotifyPlayerInitCallback;)V")
	FGL_JNI_DEF_JMETHOD(SpotifyUtils, destroyPlayer, "destroyPlayer", "(Lcom/spotify/sdk/android/player/SpotifyPlayer;)V")
	void SpotifyUtils::init(JNIEnv* env) {
		javaClass(env);
		methodID_constructor(env);
		methodID_getPlayer(env);
		methodID_destroyPlayer(env);
	}

	SpotifyUtils SpotifyUtils::newObject(JNIEnv* env) {
		return SpotifyUtils(env->NewObject(javaClass(env), methodID_constructor(env)));
	}

	void SpotifyUtils::getPlayer(JNIEnv* env, jstring clientId, jstring accessToken, jobject callback) {
		env->CallVoidMethod(value, methodID_getPlayer(env), clientId, accessToken, callback);
	}

	void SpotifyUtils::destroyPlayer(JNIEnv* env, jobject player) {
		env->CallVoidMethod(value, methodID_destroyPlayer(env), player);
	}
}
#endif

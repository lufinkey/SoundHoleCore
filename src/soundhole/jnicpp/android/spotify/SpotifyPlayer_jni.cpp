
#include <soundhole/jnicpp/android/spotify/SpotifyPlayer_jni.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	FGL_JNI_DEF_JCLASS(SpotifyPlayer, "com/spotify/sdk/android/player/SpotifyPlayer")
	FGL_JNI_DEF_JMETHOD(SpotifyPlayer, playUri, "playUri", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;Ljava/lang/String;II)V")
	FGL_JNI_DEF_JMETHOD(SpotifyPlayer, queue, "queue", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;Ljava/lang/String;)V")
	FGL_JNI_DEF_JMETHOD(SpotifyPlayer, seekToPosition, "seekToPosition", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;I)V")
	FGL_JNI_DEF_JMETHOD(SpotifyPlayer, pause, "pause", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;)V")
	FGL_JNI_DEF_JMETHOD(SpotifyPlayer, resume, "resume", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;)V")
	FGL_JNI_DEF_JMETHOD(SpotifyPlayer, getPlaybackState, "getPlaybackState", "()Lcom/spotify/sdk/android/player/PlaybackState;")
	FGL_JNI_DEF_JMETHOD(SpotifyPlayer, getMetadata, "getMetadata", "()Lcom/spotify/sdk/android/player/Metadata;")
	FGL_JNI_DEF_JMETHOD(SpotifyPlayer, skipToNext, "skipToNext", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;)V")
	FGL_JNI_DEF_JMETHOD(SpotifyPlayer, skipToPrevious, "skipToPrevious", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;)V")
	FGL_JNI_DEF_JMETHOD(SpotifyPlayer, setShuffle, "setShuffle", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;Z)V")
	FGL_JNI_DEF_JMETHOD(SpotifyPlayer, setRepeat, "setRepeat", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;Z)V")
	FGL_JNI_DEF_JMETHOD(SpotifyPlayer, isLoggedIn, "isLoggedIn", "()Z")
	FGL_JNI_DEF_JMETHOD(SpotifyPlayer, login, "login", "(Ljava/lang/String;)Z")
	FGL_JNI_DEF_JMETHOD(SpotifyPlayer, logout, "logout", "()Z")

	jboolean SpotifyPlayer::isLoggedIn(JNIEnv* env) {
		return env->CallBooleanMethod(value, methodID_isLoggedIn(env));
	}
	jboolean SpotifyPlayer::login(JNIEnv* env, jstring oauthToken) {
		return env->CallBooleanMethod(value, methodID_login(env), oauthToken);
	}
	jboolean SpotifyPlayer::logout(JNIEnv* env) {
		return env->CallBooleanMethod(value, methodID_logout(env));
	}
}
#endif

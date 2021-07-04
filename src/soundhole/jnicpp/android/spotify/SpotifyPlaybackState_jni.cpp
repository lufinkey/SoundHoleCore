
#include <soundhole/jnicpp/android/spotify/SpotifyPlaybackState_jni.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	FGL_JNI_DEF_JCLASS(SpotifyPlaybackState, "com/spotify/sdk/android/player/PlaybackState")
	FGL_JNI_DEF_JFIELD(SpotifyPlaybackState, isPlaying, "isPlaying", "Z")
	FGL_JNI_DEF_JFIELD(SpotifyPlaybackState, isRepeating, "isRepeating", "Z")
	FGL_JNI_DEF_JFIELD(SpotifyPlaybackState, isShuffling, "isShuffling", "Z")
	FGL_JNI_DEF_JFIELD(SpotifyPlaybackState, isActiveDevice, "isActiveDevice", "Z")
	FGL_JNI_DEF_JFIELD(SpotifyPlaybackState, positionMs, "positionMs", "J")
	void SpotifyPlaybackState::init(JNIEnv* env) {
		javaClass(env);
		fieldID_isPlaying(env);
		fieldID_isRepeating(env);
		fieldID_isShuffling(env);
		fieldID_isActiveDevice(env);
		fieldID_positionMs(env);
	}

	jboolean SpotifyPlaybackState::isPlaying(JNIEnv* env) {
		return env->GetBooleanField(value, fieldID_isPlaying(env));
	}
	jboolean SpotifyPlaybackState::isRepeating(JNIEnv* env) {
		return env->GetBooleanField(value, fieldID_isRepeating(env));
	}
	jboolean SpotifyPlaybackState::isShuffling(JNIEnv* env) {
		return env->GetBooleanField(value, fieldID_isShuffling(env));
	}
	jboolean SpotifyPlaybackState::isActiveDevice(JNIEnv* env) {
		return env->GetBooleanField(value, fieldID_isActiveDevice(env));
	}
	jlong SpotifyPlaybackState::getPositionMs(JNIEnv *env) {
		return env->GetLongField(value, fieldID_positionMs(env));
	}
}
#endif

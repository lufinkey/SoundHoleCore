
#include <soundhole/jnicpp/android/spotify/SpotifyMetadata_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyTrack_jni.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	FGL_JNI_DEF_JCLASS(SpotifyMetadata, "com/spotify/sdk/android/player/Metadata")
	FGL_JNI_DEF_JFIELD(SpotifyMetadata, contextName, "contextName", "Ljava/lang/String;")
	FGL_JNI_DEF_JFIELD(SpotifyMetadata, contextUri, "contextUri", "Ljava/lang/String;")
	FGL_JNI_DEF_JFIELD(SpotifyMetadata, prevTrack, "prevTrack", "Lcom/spotify/sdk/android/player/Metadata$Track;")
	FGL_JNI_DEF_JFIELD(SpotifyMetadata, currentTrack, "currentTrack", "Lcom/spotify/sdk/android/player/Metadata$Track;")
	FGL_JNI_DEF_JFIELD(SpotifyMetadata, nextTrack, "nextTrack", "Lcom/spotify/sdk/android/player/Metadata$Track;")
	void SpotifyMetadata::init(JNIEnv* env) {
		javaClass(env);
		fieldID_contextName(env);
		fieldID_contextUri(env);
		fieldID_prevTrack(env);
		fieldID_currentTrack(env);
		fieldID_nextTrack(env);
	}

	jstring SpotifyMetadata::getContextName(JNIEnv* env) const {
		return (jstring)env->GetObjectField(value, fieldID_contextName(env));
	}
	jstring SpotifyMetadata::getContextURI(JNIEnv* env) const {
		return (jstring)env->GetObjectField(value, fieldID_contextUri(env));
	}
	SpotifyTrack SpotifyMetadata::getPreviousTrack(JNIEnv* env) const {
		return SpotifyTrack(env->GetObjectField(value, fieldID_prevTrack(env)));
	}
	SpotifyTrack SpotifyMetadata::getCurrentTrack(JNIEnv* env) const {
		return SpotifyTrack(env->GetObjectField(value, fieldID_currentTrack(env)));
	}
	SpotifyTrack SpotifyMetadata::getNextTrack(JNIEnv* env) const {
		return SpotifyTrack(env->GetObjectField(value, fieldID_nextTrack(env)));
	}
}
#endif

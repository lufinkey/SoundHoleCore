
#include <soundhole/jnicpp/android/spotify/SpotifyTrack_jni.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	FGL_JNI_DEF_JCLASS(SpotifyTrack, "com/spotify/sdk/android/player/Metadata$Track")
	FGL_JNI_DEF_JFIELD(SpotifyTrack, name, "name", "Ljava/lang/String;")
	FGL_JNI_DEF_JFIELD(SpotifyTrack, uri, "uri", "Ljava/lang/String;")
	FGL_JNI_DEF_JFIELD(SpotifyTrack, artistName, "artistName", "Ljava/lang/String;")
	FGL_JNI_DEF_JFIELD(SpotifyTrack, artistUri, "artistUri", "Ljava/lang/String;")
	FGL_JNI_DEF_JFIELD(SpotifyTrack, albumName, "albumName", "Ljava/lang/String;")
	FGL_JNI_DEF_JFIELD(SpotifyTrack, albumUri, "albumUri", "Ljava/lang/String;")
	FGL_JNI_DEF_JFIELD(SpotifyTrack, durationMs, "durationMs", "J")
	FGL_JNI_DEF_JFIELD(SpotifyTrack, indexInContext, "indexInContext", "J")
	FGL_JNI_DEF_JFIELD(SpotifyTrack, albumCoverWebUrl, "albumCoverWebUrl", "Ljava/lang/String;")
	void SpotifyTrack::init(JNIEnv* env) {
		javaClass(env);
		fieldID_name(env);
		fieldID_uri(env);
		fieldID_artistName(env);
		fieldID_artistUri(env);
		fieldID_albumName(env);
		fieldID_albumUri(env);
		fieldID_durationMs(env);
		fieldID_indexInContext(env);
		fieldID_albumCoverWebUrl(env);
	}

	jstring SpotifyTrack::getName(JNIEnv* env) const {
		return (jstring)env->GetObjectField(value, fieldID_name(env));
	}
	jstring SpotifyTrack::getURI(JNIEnv* env) const {
		return (jstring)env->GetObjectField(value, fieldID_uri(env));
	}
	jstring SpotifyTrack::getArtistName(JNIEnv* env) const {
		return (jstring)env->GetObjectField(value, fieldID_artistName(env));
	}
	jstring SpotifyTrack::getArtistURI(JNIEnv* env) const {
		return (jstring)env->GetObjectField(value, fieldID_artistUri(env));
	}
	jstring SpotifyTrack::getAlbumName(JNIEnv* env) const {
		return (jstring)env->GetObjectField(value, fieldID_albumName(env));
	}
	jstring SpotifyTrack::getAlbumURI(JNIEnv* env) const {
		return (jstring)env->GetObjectField(value, fieldID_albumUri(env));
	}
	jlong SpotifyTrack::getDurationMs(JNIEnv *env) const {
		return env->GetLongField(value, fieldID_durationMs(env));
	}
	jlong SpotifyTrack::getIndexInContext(JNIEnv *env) const {
		return env->GetLongField(value, fieldID_indexInContext(env));
	}
	jstring SpotifyTrack::getAlbumCoverWebURL(JNIEnv* env) const {
		return (jstring)env->GetObjectField(value, fieldID_albumCoverWebUrl(env));
	}
}
#endif

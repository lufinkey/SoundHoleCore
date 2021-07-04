
#include <soundhole/jnicpp/android/spotify/SpotifySession_jni.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	FGL_JNI_DEF_JCLASS(SpotifySession, "com/lufinkey/soundholecore/android/spotify/SpotifySession")
	FGL_JNI_DEF_JFIELD(SpotifySession, accessToken, "accessToken", "Ljava/lang/String;")
	FGL_JNI_DEF_JFIELD(SpotifySession, expireTime, "expireTime", "J")
	FGL_JNI_DEF_JFIELD(SpotifySession, tokenType, "tokenType", "Ljava/lang/String;")
	FGL_JNI_DEF_JFIELD(SpotifySession, refreshToken, "refreshToken", "Ljava/lang/String;")
	FGL_JNI_DEF_JFIELD(SpotifySession, scopes, "scopes", "[Ljava/lang/String;")
	void SpotifySession::init(JNIEnv* env) {
		javaClass(env);
		fieldID_accessToken(env);
		fieldID_expireTime(env);
		fieldID_tokenType(env);
		fieldID_refreshToken(env);
		fieldID_scopes(env);
	}

	jstring SpotifySession::getAccessToken(JNIEnv* env) const {
		return (jstring)env->GetObjectField(value, fieldID_accessToken(env));
	}
	jlong SpotifySession::getExpireTime(JNIEnv* env) const {
		return env->GetLongField(value, fieldID_expireTime(env));
	}
	jstring SpotifySession::getTokenType(JNIEnv *env) const {
		return (jstring)env->GetObjectField(value, fieldID_tokenType(env));
	}
	jstring SpotifySession::getRefreshToken(JNIEnv* env) const {
		return (jstring)env->GetObjectField(value, fieldID_refreshToken(env));
	}
	jobjectArray SpotifySession::getScopes(JNIEnv* env) const {
		return (jobjectArray)env->GetObjectField(value, fieldID_scopes(env));
	}
}
#endif

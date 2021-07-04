
#pragma once

#include <soundhole/jnicpp/common.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	struct SpotifySession: JNIObject {
		using JNIObject::JNIObject;
		static void init(JNIEnv* env);
		static FGL_JNI_DECL_JCLASS
		static FGL_JNI_DECL_JFIELD(accessToken)
		static FGL_JNI_DECL_JFIELD(expireTime)
		static FGL_JNI_DECL_JFIELD(tokenType)
		static FGL_JNI_DECL_JFIELD(refreshToken)
		static FGL_JNI_DECL_JFIELD(scopes)

		jstring getAccessToken(JNIEnv* env) const;
		jlong getExpireTime(JNIEnv* env) const;
		jstring getTokenType(JNIEnv* env) const;
		jstring getRefreshToken(JNIEnv* env) const;
		jobjectArray getScopes(JNIEnv* env) const;
	};
}
#endif

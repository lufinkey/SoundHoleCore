
#pragma once

#include <soundhole/jnicpp/jnicpp_common.hpp>
#include <soundhole/providers/spotify/api/SpotifyAuth.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	struct SpotifyAuthenticateOptions: JNIObject {
		using JNIObject::JNIObject;
		static void init(JNIEnv* env);
		static FGL_JNI_DECL_JCLASS
		static FGL_JNI_DECL_JCONSTRUCTOR()
		static FGL_JNI_DECL_JFIELD(clientId)
		static FGL_JNI_DECL_JFIELD(clientSecret)
		static FGL_JNI_DECL_JFIELD(redirectURL)
		static FGL_JNI_DECL_JFIELD(scopes)
		static FGL_JNI_DECL_JFIELD(tokenSwapURL)
		static FGL_JNI_DECL_JFIELD(showDialog)
		static FGL_JNI_DECL_JFIELD(loginLoadingText)

		static SpotifyAuthenticateOptions newObject(JNIEnv* env);
		static SpotifyAuthenticateOptions from(JNIEnv* env, sh::SpotifyAuth::AuthenticateOptions options);

		void setClientId(JNIEnv* env, jstring clientId);
		void setClientSecret(JNIEnv* env, jstring clientSecret);
		void setRedirectURL(JNIEnv* env, jstring redirectURL);
		void setScopes(JNIEnv* env, jobjectArray scopes);
		void setTokenSwapURL(JNIEnv* env, jstring tokenSwapURL);
		void setShowDialog(JNIEnv* env, Optional<jboolean> showDialog);
		void setLoginLoadingText(JNIEnv* env, jstring loginLoadingText);
	};
}
#endif

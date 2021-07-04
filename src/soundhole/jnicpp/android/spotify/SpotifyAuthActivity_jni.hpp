
#pragma once

#include <soundhole/jnicpp/common.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyAuthenticateOptions_jni.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	struct SpotifyAuthActivity: JNIObject {
		using JNIObject::JNIObject;
		static void init(JNIEnv* env);
		static FGL_JNI_DECL_JCLASS
		static FGL_JNI_DECL_STATIC_JMETHOD(performAuthFlow)
		static FGL_JNI_DECL_JMETHOD(finish)
		static FGL_JNI_DECL_JMETHOD(showProgressDialog)
		static FGL_JNI_DECL_JMETHOD(hideProgressDialog)

		static void performAuthFlow(JNIEnv* env, jobject context, SpotifyAuthenticateOptions authOptions, jobject listener);

		void finish(JNIEnv* env, jobject completion);
		void showProgressDialog(JNIEnv* env, jstring loadingText);
		void hideProgressDialog(JNIEnv* env);
	};
}
#endif

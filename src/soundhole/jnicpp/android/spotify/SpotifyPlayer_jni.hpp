
#pragma once

#include <soundhole/jnicpp/common.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	struct SpotifyPlayer: JNIObject {
		using JNIObject::JNIObject;
		static void init(JNIEnv* env);
		static FGL_JNI_DECL_JCLASS
		static FGL_JNI_DECL_JMETHOD(playUri)
		static FGL_JNI_DECL_JMETHOD(queue)
		static FGL_JNI_DECL_JMETHOD(seekToPosition)
		static FGL_JNI_DECL_JMETHOD(pause)
		static FGL_JNI_DECL_JMETHOD(resume)
		static FGL_JNI_DECL_JMETHOD(getPlaybackState)
		static FGL_JNI_DECL_JMETHOD(getMetadata)
		static FGL_JNI_DECL_JMETHOD(skipToNext)
		static FGL_JNI_DECL_JMETHOD(skipToPrevious)
		static FGL_JNI_DECL_JMETHOD(setShuffle)
		static FGL_JNI_DECL_JMETHOD(setRepeat)
		static FGL_JNI_DECL_JMETHOD(isLoggedIn)
		static FGL_JNI_DECL_JMETHOD(login)
		static FGL_JNI_DECL_JMETHOD(logout)

		jboolean isLoggedIn(JNIEnv* env);
		jboolean login(JNIEnv* env, jstring oauthToken);
		jboolean logout(JNIEnv* env);
	};
}
#endif

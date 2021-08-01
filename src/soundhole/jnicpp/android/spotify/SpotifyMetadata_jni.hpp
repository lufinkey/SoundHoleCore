
#pragma once

#include <soundhole/jnicpp/jnicpp_common.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	struct SpotifyTrack;

	struct SpotifyMetadata: JNIObject {
		using JNIObject::JNIObject;
		static void init(JNIEnv* env);
		static FGL_JNI_DECL_JCLASS
		static FGL_JNI_DECL_JFIELD(contextName)
		static FGL_JNI_DECL_JFIELD(contextUri)
		static FGL_JNI_DECL_JFIELD(prevTrack)
		static FGL_JNI_DECL_JFIELD(currentTrack)
		static FGL_JNI_DECL_JFIELD(nextTrack)

		jstring getContextName(JNIEnv* env) const;
		jstring getContextURI(JNIEnv* env) const;
		SpotifyTrack getPreviousTrack(JNIEnv* env) const;
		SpotifyTrack getCurrentTrack(JNIEnv* env) const;
		SpotifyTrack getNextTrack(JNIEnv* env) const;
	};
}
#endif

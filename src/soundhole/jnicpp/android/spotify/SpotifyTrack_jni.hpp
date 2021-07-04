
#pragma once

#include <soundhole/jnicpp/common.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	struct SpotifyTrack: JNIObject {
		using JNIObject::JNIObject;
		static void init(JNIEnv* env);
		static FGL_JNI_DECL_JCLASS
		static FGL_JNI_DECL_JFIELD(name)
		static FGL_JNI_DECL_JFIELD(uri)
		static FGL_JNI_DECL_JFIELD(artistName)
		static FGL_JNI_DECL_JFIELD(artistUri)
		static FGL_JNI_DECL_JFIELD(albumName)
		static FGL_JNI_DECL_JFIELD(albumUri)
		static FGL_JNI_DECL_JFIELD(durationMs)
		static FGL_JNI_DECL_JFIELD(indexInContext)
		static FGL_JNI_DECL_JFIELD(albumCoverWebUrl)

		jstring getName(JNIEnv* env) const;
		jstring getURI(JNIEnv* env) const;
		jstring getArtistName(JNIEnv* env) const;
		jstring getArtistURI(JNIEnv* env) const;
		jstring getAlbumName(JNIEnv* env) const;
		jstring getAlbumURI(JNIEnv* env) const;
		jlong getDurationMs(JNIEnv* env) const;
		jlong getIndexInContext(JNIEnv* env) const;
		jstring getAlbumCoverWebURL(JNIEnv* env) const;
	};
}
#endif

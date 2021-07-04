
#pragma once

#include <soundhole/jnicpp/common.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android {
	struct MediaPlayer: JNIObject {
		using JNIObject::JNIObject;
		static void init(JNIEnv* env);
		static FGL_JNI_DECL_JCLASS
		static FGL_JNI_DECL_JCONSTRUCTOR()
		static FGL_JNI_DECL_JMETHOD(setAudioStreamType)
		static FGL_JNI_DECL_JMETHOD(setDataSource)
		static FGL_JNI_DECL_JMETHOD(prepare)
		static FGL_JNI_DECL_JMETHOD(prepareAsync)
		static FGL_JNI_DECL_JMETHOD(start)
		static FGL_JNI_DECL_JMETHOD(pause)
		static FGL_JNI_DECL_JMETHOD(seekTo)
		static FGL_JNI_DECL_JMETHOD(stop)
		static FGL_JNI_DECL_JMETHOD(reset)
		static FGL_JNI_DECL_JMETHOD(isPlaying)
		static FGL_JNI_DECL_JMETHOD(isLooping)
		static FGL_JNI_DECL_JMETHOD(getCurrentPosition)
		static FGL_JNI_DECL_JMETHOD(getDuration)

		static MediaPlayer newObject(JNIEnv* env);

		void setAudioStreamType(JNIEnv* env, jint type);
		void setDataSource(JNIEnv* env, jstring dataSource);
		void prepare(JNIEnv* env);
		void prepareAsync(JNIEnv* env);
		void start(JNIEnv* env);
		void pause(JNIEnv* env);
		void seekTo(JNIEnv* env, jint position);
		void stop(JNIEnv* env);
		void reset(JNIEnv* env);
		jboolean isPlaying(JNIEnv* env);
		jboolean isLooping(JNIEnv* env);
		jint getCurrentPosition(JNIEnv* env);
		jint getDuration(JNIEnv* env);
	};
}
#endif

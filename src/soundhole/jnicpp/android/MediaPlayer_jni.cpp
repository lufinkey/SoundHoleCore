
#include <soundhole/jnicpp/android/MediaPlayer_jni.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android {
	FGL_JNI_DEF_JCLASS(MediaPlayer, "android/media/MediaPlayer")
	FGL_JNI_DEF_JCONSTRUCTOR(MediaPlayer, , "()V")
	FGL_JNI_DEF_JMETHOD(MediaPlayer, setAudioStreamType, "setAudioStreamType", "(I)V")
	FGL_JNI_DEF_JMETHOD(MediaPlayer, setDataSource, "setDataSource", "(Ljava/lang/String;)V")
	FGL_JNI_DEF_JMETHOD(MediaPlayer, prepare, "prepare", "()V")
	FGL_JNI_DEF_JMETHOD(MediaPlayer, prepareAsync, "prepareAsync", "()V")
	FGL_JNI_DEF_JMETHOD(MediaPlayer, start, "start", "()V")
	FGL_JNI_DEF_JMETHOD(MediaPlayer, pause, "pause", "()V")
	FGL_JNI_DEF_JMETHOD(MediaPlayer, seekTo, "seekTo", "(I)V")
	FGL_JNI_DEF_JMETHOD(MediaPlayer, stop, "stop", "()V")
	FGL_JNI_DEF_JMETHOD(MediaPlayer, reset, "reset", "()V")
	FGL_JNI_DEF_JMETHOD(MediaPlayer, isPlaying, "isPlaying", "()Z")
	FGL_JNI_DEF_JMETHOD(MediaPlayer, isLooping, "isLooping", "()Z")
	FGL_JNI_DEF_JMETHOD(MediaPlayer, getCurrentPosition, "getCurrentPosition", "()I")
	FGL_JNI_DEF_JMETHOD(MediaPlayer, getDuration, "getDuration", "()I")
	void MediaPlayer::init(JNIEnv* env) {
		javaClass(env);
		methodID_setAudioStreamType(env);
		methodID_setDataSource(env);
		methodID_prepare(env);
		methodID_prepareAsync(env);
		methodID_start(env);
		methodID_pause(env);
		methodID_seekTo(env);
		methodID_stop(env);
		methodID_reset(env);
		methodID_isPlaying(env);
		methodID_isLooping(env);
		methodID_getCurrentPosition(env);
		methodID_getDuration(env);
	}

	MediaPlayer MediaPlayer::newObject(JNIEnv* env) {
		return MediaPlayer(env->NewObject(javaClass(env), methodID_constructor(env)));
	}

	void MediaPlayer::setAudioStreamType(JNIEnv *env, jint type) {
		env->CallVoidMethod(value, methodID_setAudioStreamType(env), type);
	}
	void MediaPlayer::setDataSource(JNIEnv* env, jstring dataSource) {
		env->CallVoidMethod(value, methodID_setDataSource(env), dataSource);
	}
	void MediaPlayer::prepare(JNIEnv* env) {
		env->CallVoidMethod(value, methodID_prepare(env));
	}
	void MediaPlayer::prepareAsync(JNIEnv* env) {
		env->CallVoidMethod(value, methodID_prepareAsync(env));
	}
	void MediaPlayer::start(JNIEnv* env) {
		env->CallVoidMethod(value, methodID_start(env));
	}
	void MediaPlayer::pause(JNIEnv* env) {
		env->CallVoidMethod(value, methodID_pause(env));
	}
	void MediaPlayer::seekTo(JNIEnv* env, jint position) {
		env->CallVoidMethod(value, methodID_seekTo(env), position);
	}
	void MediaPlayer::stop(JNIEnv* env) {
		env->CallVoidMethod(value, methodID_stop(env));
	}
	void MediaPlayer::reset(JNIEnv* env) {
		env->CallVoidMethod(value, methodID_reset(env));
	}
	jboolean MediaPlayer::isPlaying(JNIEnv* env) {
		return env->CallBooleanMethod(value, methodID_isPlaying(env));
	}
	jboolean MediaPlayer::isLooping(JNIEnv* env) {
		return env->CallBooleanMethod(value, methodID_isLooping(env));
	}
	jint MediaPlayer::getCurrentPosition(JNIEnv* env) {
		return env->CallIntMethod(value, methodID_getCurrentPosition(env));
	}
	jint MediaPlayer::getDuration(JNIEnv* env) {
		return env->CallIntMethod(value, methodID_getDuration(env));
	}
}
#endif

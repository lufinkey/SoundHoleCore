
#include <soundhole/jnicpp/android/AudioManager_jni.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android {
	FGL_JNI_DEF_JCLASS(AudioManager, "android/media/AudioManager")
	FGL_JNI_DEF_STATIC_JFIELD(AudioManager, STREAM_MUSIC, "STREAM_MUSIC", "I")

	jint AudioManager::STREAM_MUSIC(JNIEnv* env) {
		return env->GetStaticIntField(javaClass(env), fieldID_static_STREAM_MUSIC(env));
	}
}
#endif


#include <soundhole/jnicpp/SoundHole_jni.hpp>
#include <soundhole/jnicpp/NativeFunction_jni.hpp>
#include <soundhole/jnicpp/NativeCallback_jni.hpp>
#include <soundhole/jnicpp/android/AudioManager_jni.hpp>
#include <soundhole/jnicpp/android/MediaPlayer_jni.hpp>
#include <soundhole/jnicpp/android/Utils_jni.hpp>
#include <soundhole/jnicpp/android/spotify/NativeSpotifyAuthActivityListener_jni.hpp>
#include <soundhole/jnicpp/android/spotify/NativeSpotifyPlayerInitCallback_jni.hpp>
#include <soundhole/jnicpp/android/spotify/NativeSpotifyPlayerOpCallback_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyAuthActivity_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyAuthenticateOptions_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyError_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyMetadata_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyPlaybackState_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyPlayer_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyPlayerEventHandler_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifySession_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyTrack_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyUtils_jni.hpp>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#ifdef SOUNDHOLE_JNI_ENABLED

namespace sh::jni {
	using namespace android;
	using namespace android::spotify;
}

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	__android_log_print(ANDROID_LOG_DEBUG, "IOCpp", "JNI module initialized");
	fgl::jniScope(vm, [](JNIEnv* env) {
		namespace jni = sh::jni;
		jni::NativeFunction::init(env);
		jni::NativeCallback::init(env);
		jni::AudioManager::init(env);
		jni::MediaPlayer::init(env);
		jni::Utils::init(env);
		jni::NativeSpotifyAuthActivityListener::init(env);
		jni::NativeSpotifyPlayerInitCallback::init(env);
		jni::NativeSpotifyPlayerOpCallback::init(env);
		jni::SpotifyAuthActivity::init(env);
		jni::SpotifyAuthenticateOptions::init(env);
		jni::SpotifyError::init(env);
		jni::SpotifyMetadata::init(env);
		jni::SpotifyPlaybackState::init(env);
		jni::SpotifyPlayer::init(env);
		jni::SpotifyPlayerEventHandler::init(env);
		jni::SpotifySession::init(env);
		jni::SpotifyTrack::init(env);
		jni::SpotifyUtils::init(env);
	});
	return JNI_VERSION_1_6;
}

namespace sh {
	JNIObject::JNIObject()
		: value(nullptr) {
		//
	}

	JNIObject::JNIObject(jobject val)
		: value(val) {
		//
	}

	JNIObject& JNIObject::operator=(jobject val) {
		value = val;
		return *this;
	}

	JNIObject& JNIObject::operator=(std::nullptr_t) {
		value = nullptr;
		return *this;
	}

	JNIObject::operator jobject() const {
		return value;
	}
}

#endif

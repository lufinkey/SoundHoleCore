
#pragma once

#include <soundhole/jnicpp/jnicpp_common.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	struct SpotifyPlayerEventHandler: JNIObject {
		using JNIObject::JNIObject;
		static void init(JNIEnv* env);
		static FGL_JNI_DECL_JCLASS
		static FGL_JNI_DECL_JCONSTRUCTOR()
		static FGL_JNI_DECL_JMETHOD(destroy)

		struct ConstructorParams {
			jobject player;
			Function<void(JNIEnv*)> onLoggedIn;
			Function<void(JNIEnv*)> onLoggedOut;
			Function<void(JNIEnv*,jobject)> onLoginFailed;
			Function<void(JNIEnv*)> onTemporaryError;
			Function<void(JNIEnv*,jstring)> onConnectionMessage;
			Function<void(JNIEnv*)> onDisconnect;
			Function<void(JNIEnv*)> onReconnect;
			Function<void(JNIEnv*,jobject)> onPlaybackEvent;
			Function<void(JNIEnv*,jobject)> onPlaybackError;
		};

		static SpotifyPlayerEventHandler newObject(JNIEnv* env, ConstructorParams params);
		void destroy(JNIEnv* env);
	};
}
#endif

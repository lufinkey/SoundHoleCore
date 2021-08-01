
#pragma once

#include <soundhole/jnicpp/jnicpp_common.hpp>
#include <soundhole/providers/spotify/api/SpotifyAuth.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	struct SpotifyAuthActivity;

	struct NativeSpotifyAuthActivityListener: JNIObject {
		using JNIObject::JNIObject;
		static void init(JNIEnv* env);
		static FGL_JNI_DECL_JCLASS;
		static FGL_JNI_DECL_JCONSTRUCTOR()

		struct ConstructorParams {
			Function<void(JNIEnv*,SpotifyAuthActivity,sh::SpotifySession)> onReceiveSession;
			Function<void(JNIEnv*,SpotifyAuthActivity,String)> onReceiveCode;
			Function<void(JNIEnv*,SpotifyAuthActivity)> onCancel;
			Function<void(JNIEnv*,SpotifyAuthActivity,String)> onFailure;
		};
		static NativeSpotifyAuthActivityListener newObject(JNIEnv* env, ConstructorParams params);
	};
}
#endif

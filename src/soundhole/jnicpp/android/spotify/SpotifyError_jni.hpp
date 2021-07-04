
#pragma once

#include <soundhole/jnicpp/common.hpp>
#include <soundhole/providers/spotify/api/SpotifyError.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	struct SpotifyError: JNIObject {
		using JNIObject::JNIObject;
		static void init(JNIEnv* env);
		static FGL_JNI_DECL_JCLASS
		static FGL_JNI_DECL_JFIELD(nativeCode)

		static std::map<jint,std::tuple<sh::SpotifyError::Code,String>> enumMap;
		static SpotifyError getEnum(JNIEnv* env, const char* name);

		jint getNativeCode(JNIEnv* env);
	};
}
#endif

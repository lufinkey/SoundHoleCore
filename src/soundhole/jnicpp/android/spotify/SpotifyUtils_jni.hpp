
#include <soundhole/jnicpp/jnicpp_common.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	struct SpotifyUtils: JNIObject {
		using JNIObject::JNIObject;
		static void init(JNIEnv* env);
		static FGL_JNI_DECL_JCLASS
		static FGL_JNI_DECL_JCONSTRUCTOR()
		static FGL_JNI_DECL_JMETHOD(getPlayer)
		static FGL_JNI_DECL_JMETHOD(destroyPlayer)

		static SpotifyUtils newObject(JNIEnv* env);
		void getPlayer(JNIEnv* env, jstring clientId, jstring accessToken, jobject callback);
		void destroyPlayer(JNIEnv* env, jobject player);
	};
}
#endif

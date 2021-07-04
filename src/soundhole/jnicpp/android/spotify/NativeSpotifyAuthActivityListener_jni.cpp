
#include <soundhole/jnicpp/android/spotify/NativeSpotifyAuthActivityListener_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyAuthActivity_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifySession_jni.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	FGL_JNI_DEF_JCLASS(NativeSpotifyAuthActivityListener, "com/lufinkey/soundholecore/android/spotify/NativeSpotifyAuthActivityListener")
	FGL_JNI_DEF_JCONSTRUCTOR(NativeSpotifyAuthActivityListener, , "(JJJJ)V")
	void NativeSpotifyAuthActivityListener::init(JNIEnv* env) {
		javaClass(env);
		methodID_constructor(env);
	}

	NativeSpotifyAuthActivityListener NativeSpotifyAuthActivityListener::newObject(JNIEnv* env, ConstructorParams params) {
		auto onReceiveSession = params.onReceiveSession;
		auto onReceiveCode = params.onReceiveCode;
		auto onCancel = params.onCancel;
		auto onFailure = params.onFailure;
		return NativeSpotifyAuthActivityListener(env->NewObject(javaClass(env), methodID_constructor(env),
			(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				auto activity = SpotifyAuthActivity(args[0]);
				auto session = jni::android::spotify::SpotifySession(args[1]);
				jstring refreshToken = session.getRefreshToken(env);
				jstring tokenType = session.getTokenType(env);
				jobjectArray scopesArray = session.getScopes(env);
				onReceiveSession(env, activity, sh::SpotifySession(
					String(env, session.getAccessToken(env)),
					std::chrono::system_clock::from_time_t((time_t)(session.getExpireTime(env) / 1000)),
					(tokenType != nullptr) ? String(env, tokenType) : String("Bearer"),
					(refreshToken != nullptr) ? String(env, refreshToken) : String(),
					ArrayList<String>(env, scopesArray, [](JNIEnv* env, jobject scope) {
						return String(env, (jstring)scope);
					})
				));
			})),
			(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				auto activity = SpotifyAuthActivity(args[0]);
				jstring code = (jstring)args[1];
				onReceiveCode(env, activity, String(env, code));
			})),
			(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				auto activity = SpotifyAuthActivity(args[0]);
				onCancel(env, activity);
			})),
			(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				auto activity = SpotifyAuthActivity(args[0]);
				jstring error = (jstring)args[1];
				onReceiveCode(env, activity, String(env, error));
			}))));
	}
}
#endif

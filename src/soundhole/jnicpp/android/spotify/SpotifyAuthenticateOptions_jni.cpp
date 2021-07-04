
#include <soundhole/jnicpp/android/spotify/SpotifyAuthenticateOptions_jni.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	FGL_JNI_DEF_JCLASS(SpotifyAuthenticateOptions, "com/lufinkey/soundholecore/android/spotify/SpotifyAuthenticateOptions")
	FGL_JNI_DEF_JCONSTRUCTOR(SpotifyAuthenticateOptions, , "()V")
	FGL_JNI_DEF_JFIELD(SpotifyAuthenticateOptions, clientId, "clientId", "Ljava/lang/String;")
	FGL_JNI_DEF_JFIELD(SpotifyAuthenticateOptions, clientSecret, "clientSecret", "Ljava/lang/String;")
	FGL_JNI_DEF_JFIELD(SpotifyAuthenticateOptions, redirectURL, "redirectURL", "Ljava/lang/String;")
	FGL_JNI_DEF_JFIELD(SpotifyAuthenticateOptions, scopes, "scopes", "[Ljava/lang/String;")
	FGL_JNI_DEF_JFIELD(SpotifyAuthenticateOptions, tokenSwapURL, "tokenSwapURL", "Ljava/lang/String;")
	FGL_JNI_DEF_JFIELD(SpotifyAuthenticateOptions, showDialog, "showDialog", "Ljava/lang/Boolean;")
	FGL_JNI_DEF_JFIELD(SpotifyAuthenticateOptions, loginLoadingText, "loginLoadingText", "Ljava/lang/String;")
	void SpotifyAuthenticateOptions::init(JNIEnv* env) {
		javaClass(env);
		methodID_constructor(env);
		fieldID_clientId(env);
		fieldID_clientSecret(env);
		fieldID_redirectURL(env);
		fieldID_scopes(env);
		fieldID_tokenSwapURL(env);
		fieldID_showDialog(env);
		fieldID_loginLoadingText(env);
	}

	SpotifyAuthenticateOptions SpotifyAuthenticateOptions::newObject(JNIEnv* env) {
		return SpotifyAuthenticateOptions(env->NewObject(javaClass(env), methodID_constructor(env)));
	}

	SpotifyAuthenticateOptions SpotifyAuthenticateOptions::from(JNIEnv* env, sh::SpotifyAuth::AuthenticateOptions options) {
		jclass stringClass = env->FindClass("java/lang/String");
		auto authOptions = newObject(env);
		authOptions.setClientId(env, options.clientId.toJavaString(env));
		authOptions.setClientSecret(env, options.clientSecret.toJavaString(env));
		authOptions.setRedirectURL(env, options.redirectURL.toJavaString(env));
		authOptions.setScopes(env, options.scopes.toJavaObjectArray(env, stringClass, [](JNIEnv* env, auto& scope) {
			return scope.toJavaString(env);
		}));
		authOptions.setTokenSwapURL(env, options.tokenSwapURL.toJavaString(env));
		authOptions.setShowDialog(env, options.showDialog.map([](auto val) { return (jboolean)val; }));
		authOptions.setLoginLoadingText(env, options.android.loginLoadingText.toJavaString(env));
		return authOptions;
	}

	void SpotifyAuthenticateOptions::setClientId(JNIEnv* env, jstring clientId) {
		env->SetObjectField(value, fieldID_clientId(env), clientId);
	}
	void SpotifyAuthenticateOptions::setClientSecret(JNIEnv* env, jstring clientSecret) {
		env->SetObjectField(value, fieldID_clientSecret(env), clientSecret);
	}
	void SpotifyAuthenticateOptions::setRedirectURL(JNIEnv* env, jstring redirectURL) {
		env->SetObjectField(value, fieldID_redirectURL(env), redirectURL);
	}
	void SpotifyAuthenticateOptions::setScopes(JNIEnv* env, jobjectArray scopes) {
		env->SetObjectField(value, fieldID_scopes(env), scopes);
	}
	void SpotifyAuthenticateOptions::setTokenSwapURL(JNIEnv* env, jstring tokenSwapURL) {
		env->SetObjectField(value, fieldID_tokenSwapURL(env), tokenSwapURL);
	}
	void SpotifyAuthenticateOptions::setShowDialog(JNIEnv* env, Optional<jboolean> showDialog) {
		if(showDialog.hasValue()) {
			env->SetObjectField(value, fieldID_showDialog(env), nullptr);
		} else {
			jclass boolClass = env->FindClass("java/lang/Boolean");
			jmethodID boolConstructor = env->GetMethodID(boolClass, "<init>", "(Z)V");
			jobject boolVal = env->NewObject(boolClass, boolConstructor, (jboolean)showDialog.value());
			env->SetObjectField(value, fieldID_showDialog(env), boolVal);
		}
	}
	void SpotifyAuthenticateOptions::setLoginLoadingText(JNIEnv* env, jstring loginLoadingText) {
		env->SetObjectField(value, fieldID_loginLoadingText(env), loginLoadingText);
	}
}
#endif

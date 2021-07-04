
#include <soundhole/jnicpp/android/spotify/SpotifyAuthActivity_jni.hpp>

#ifdef SOUNDHOLE_JNI_DISABLED
namespace sh::jni::android::spotify {
	FGL_JNI_DEF_JCLASS(SpotifyAuthActivity, "com/lufinkey/soundholecore/android/spotify/SpotifyAuthActivity")
	FGL_JNI_DECL_STATIC_JMETHOD(SpotifyAuthActivity, performAuthFlow, "performAuthFlow", "(Landroid/content/Context;Lcom/lufinkey/soundholecore/SpotifyLoginOptions;Lcom/lufinkey/soundholecore/SpotifyAuthActivityListener;)V")
	FGL_JNI_DECL_JMETHOD(SpotifyAuthActivity, finish, "finish", "(Lcom/lufinkey/soundholecore/NativeFunction;)V")
	FGL_JNI_DECL_JMETHOD(SpotifyAuthActivity, showProgressDialog, "showProgressDialog", "(Ljava/lang/String;)V")
	FGL_JNI_DECL_JMETHOD(SpotifyAuthActivity, hideProgressDialog, "hideProgressDialog", "()V")
	void SpotifyAuthActivity::init(JNIEnv* env) {
		javaClass(env);
		methodID_static_performAuthFlow(env);
		methodID_finish(env);
		methodID_showProgressDialog(env);
		methodID_hideProgressDialog(env);
	}

	void SpotifyAuthActivity::performAuthFlow(JNIEnv* env, jobject context, jobject loginOptions, jobject listener) {
		env->CallStaticVoidMethod(javaClass(env), methodID_static_performAuthFlow(env), context, loginOptions, listener);
	}

	void SpotifyAuthActivity::finish(JNIEnv* env, jobject completion) {
		env->CallVoidMethod(value, methodID_finish(env), completion);
	}

	void SpotifyAuthActivity::showProgressDialog(JNIEnv* env, jobject self, jstring loadingText) {
		env->CallVoidMethod(value, methodID_showProgressDialog(env), loadingText);
	}

	void SpotifyAuthActivity::hideProgressDialog(JNIEnv* env, jobject self) {
		env->CallVoidMethod(value, methodID_hideProgressDialog(env));
	}
}
#endif

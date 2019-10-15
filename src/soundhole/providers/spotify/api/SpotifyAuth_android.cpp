//
//  SpotifyAuth_android.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/7/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#ifdef __ANDROID__

#include <jni.h>
#include "SpotifyAuth.hpp"
#include "SpotifyError.hpp"
#include <soundhole/utils/android/AndroidUtils.hpp>

#ifdef TARGETPLATFORM_ANDROID

namespace sh {
	Promise<Optional<SpotifySession>> SpotifyAuth::authenticate(Options options) {
		if(options.params.find("show_dialog") == options.params.end()) {
			options.params["show_dialog"] = "true";
		}
		JavaVM* vm = getMainJavaVM();
		if(vm == nullptr) {
			throw std::runtime_error("Java VM has not been initialized");
		}
		ScopedJNIEnv scopedEnv(vm);
		JNIEnv* env = scopedEnv.getEnv();
		jobject context = android::Utils::getAppContext(env);
		if(context == nullptr) {
			throw std::runtime_error("SoundHole.mainActivity has not been set");
		}
		context = env->NewGlobalRef(context);
		return Promise<Optional<SpotifySession>>([=](auto resolve, auto reject) {
			android::Utils::runOnMainThread(env, [=](JNIEnv* env) {
				android::SpotifyAuthActivity::performAuthFlow(env,
					context, android::SpotifyLoginOptions::from(env, options),
					android::SpotifyNativeAuthActivityListener::newObject(env, {
						.onReceiveSession = [=](JNIEnv* env, jobject activity, sh::SpotifySession session) {
							android::SpotifyAuthActivity::finish(env, activity,
								android::NativeFunction::newObject(env, [=](JNIEnv* env, std::vector<jobject> args) {
									resolve(session);
								}));
						},
						.onReceiveCode = [=](JNIEnv* env, jobject activity, String code) {
							auto loadingText = options.android.loginLoadingText;
							if(loadingText.empty()) {
								loadingText = "Loading...";
							}
							android::SpotifyAuthActivity::showProgressDialog(env, activity, loadingText.toJavaString(env));
							activity = env->NewGlobalRef(activity);
							sh::SpotifyAuth::swapCodeForToken(code, options.tokenSwapURL).then([=](SpotifySession session) {
								ScopedJNIEnv scopedEnv(getMainJavaVM());
								JNIEnv* env = scopedEnv.getEnv();
								android::Utils::runOnMainThread(env, [=](JNIEnv* env) {
									android::SpotifyAuthActivity::hideProgressDialog(env, activity);
									android::SpotifyAuthActivity::finish(env, activity,
										android::NativeFunction::newObject(env, [=](JNIEnv* env, std::vector<jobject> args) {
											resolve(session);
										}));
									env->DeleteGlobalRef(activity);
								});
							}).except([=](std::exception_ptr error) {
								ScopedJNIEnv scopedEnv(getMainJavaVM());
								JNIEnv* env = scopedEnv.getEnv();
								android::Utils::runOnMainThread(env, [=](JNIEnv* env) {
									android::SpotifyAuthActivity::hideProgressDialog(env, activity);
									android::SpotifyAuthActivity::finish(env, activity,
										android::NativeFunction::newObject(env, [=](JNIEnv *env, std::vector<jobject> args) {
											reject(error);
										}));
									env->DeleteGlobalRef(activity);
								});
							});
						},
						.onCancel = [=](JNIEnv* env, jobject activity) {
							android::SpotifyAuthActivity::finish(env, activity,
								android::NativeFunction::newObject(env, [=](JNIEnv* env, std::vector<jobject> args) {
									resolve(std::nullopt);
								}));
						},
						.onFailure = [=](JNIEnv* env, jobject activity, String error) {
							android::SpotifyAuthActivity::finish(env, activity,
								android::NativeFunction::newObject(env, [=](JNIEnv* env, std::vector<jobject> args) {
									reject(SpotifyError(SpotifyError::Code::SDK_FAILED, error));
								}));
						}
					})
				);
				env->DeleteGlobalRef(context);
			});
		});
	}
}

#endif
#endif

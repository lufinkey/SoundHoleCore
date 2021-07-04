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
#include <soundhole/jnicpp/NativeFunction_jni.hpp>
#include <soundhole/jnicpp/android/Utils_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyAuthActivity_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyAuthenticateOptions_jni.hpp>
#include <soundhole/jnicpp/android/spotify/NativeSpotifyAuthActivityListener_jni.hpp>

namespace sh {
	namespace jni {
		using namespace android::spotify;
	}

	Promise<Optional<SpotifySession>> SpotifyAuth::authenticate(AuthenticateOptions options) {
		if(!options.showDialog.hasValue()) {
			options.showDialog = true;
		}
		JavaVM* vm = getJavaVM();
		if(vm == nullptr) {
			throw std::runtime_error("Java VM has not been initialized");
		}
		return Promise<Optional<SpotifySession>>([=](auto resolve, auto reject) {
			jniScope(vm, [=](JNIEnv* env) {
				jobject context = jni::android::Utils::getAppContext(env);
				if(context == nullptr) {
					throw std::runtime_error("SoundHole.mainActivity has not been set");
				}
				context = env->NewGlobalRef(context);

				jni::android::Utils::runOnMainThread(env, [=](JNIEnv* env) {
					jni::SpotifyAuthActivity::performAuthFlow(env, context,
						jni::SpotifyAuthenticateOptions::from(env, options),
						jni::NativeSpotifyAuthActivityListener::newObject(env, {
							.onReceiveSession = [=](JNIEnv* env, auto activity, sh::SpotifySession session) {
								activity.finish(env, jni::NativeFunction::newObject(env, [=](JNIEnv* env, std::vector<jobject> args) {
									resolve(session);
								}));
							},
							.onReceiveCode = [=](JNIEnv* env, auto activity, String code) {
								auto loadingText = options.android.loginLoadingText;
								if(loadingText.empty()) {
									loadingText = "Loading...";
								}
								activity.showProgressDialog(env, loadingText.toJavaString(env));
								auto activity_global = env->NewGlobalRef(activity.value);
								sh::SpotifyAuth::swapCodeForToken(code, options).then([=](SpotifySession session) {
									jniScope(getJavaVM(), [=](JNIEnv* env) {
										jni::android::Utils::runOnMainThread(env, [=](JNIEnv* env) {
											auto activity = jni::SpotifyAuthActivity(activity_global);
											activity.hideProgressDialog(env);
											activity.finish(env, jni::NativeFunction::newObject(env, [=](JNIEnv* env, std::vector<jobject> args) {
												resolve(session);
											}));
											env->DeleteGlobalRef(activity.value);
										});
									});
								}).except([=](std::exception_ptr error) {
									jniScope(getJavaVM(), [=](JNIEnv* env) {
										jni::android::Utils::runOnMainThread(env, [=](JNIEnv* env) {
											auto activity = jni::SpotifyAuthActivity(activity_global);
											activity.hideProgressDialog(env);
											activity.finish(env, jni::NativeFunction::newObject(env, [=](JNIEnv *env, std::vector<jobject> args) {
												reject(error);
											}));
											env->DeleteGlobalRef(activity.value);
										});
									});
								});
							},
							.onCancel = [=](JNIEnv* env, auto activity) {
								activity.finish(env, jni::NativeFunction::newObject(env, [=](JNIEnv* env, std::vector<jobject> args) {
									resolve(std::nullopt);
								}));
							},
							.onFailure = [=](JNIEnv* env, auto activity, String error) {
								activity.finish(env, jni::NativeFunction::newObject(env, [=](JNIEnv* env, std::vector<jobject> args) {
									reject(SpotifyError(SpotifyError::Code::SDK_FAILED, error));
								}));
							}
						})
					);
					env->DeleteGlobalRef(context);
				});
			});
		});
	}
}

#endif

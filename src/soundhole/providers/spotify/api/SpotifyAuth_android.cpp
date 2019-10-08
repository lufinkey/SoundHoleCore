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
#include <embed/nodejs/NodeJS_jni.hpp>

#ifdef TARGETPLATFORM_ANDROID

namespace sh {
	using ScopedJNIEnv = embed::nodejs::ScopedJNIEnv;

	namespace android {
		namespace SpotifySession {
			jclass javaClass = nullptr;
			jfieldID accessToken = nullptr;
			jfieldID expireTime = nullptr;
			jfieldID refreshToken = nullptr;
			jfieldID scopes = nullptr;

			inline std::vector<jfieldID> fields() {
				return { accessToken, expireTime, refreshToken, scopes };
			}
		}

		namespace SpotifyLoginOptions {
			jclass javaClass = nullptr;
			jmethodID instantiate = nullptr;
			jfieldID clientId = nullptr;
			jfieldID redirectURL = nullptr;
			jfieldID scopes = nullptr;
			jfieldID tokenSwapURL = nullptr;
			jfieldID tokenRefreshURL = nullptr;
			jfieldID params = nullptr;

			inline std::vector<jmethodID> methods() {
				return { instantiate };
			}

			inline std::vector<jfieldID> fields() {
				return { clientId, redirectURL, scopes, tokenSwapURL, tokenRefreshURL, params };
			}

			jobject from(JNIEnv* env, sh::SpotifyAuth::Options options) {
				jobject loginOptions = env->NewObject(javaClass, instantiate);
				jclass stringClass = env->FindClass("java/lang/String");
				jclass hashMapClass = env->FindClass("java/util/HashMap");
				if(stringClass == nullptr) {
					throw std::runtime_error("Could not find android String");
				} else if(hashMapClass == nullptr) {
					throw std::runtime_error("Could not find android HashMap");
				}
				jmethodID hashMapConstructor = env->GetMethodID(hashMapClass, "<init>", "()V");
				jmethodID hashMapPut = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
				if(hashMapConstructor == nullptr) {
					throw std::runtime_error("Could not find android HashMap()");
				} else if(hashMapPut == nullptr) {
					throw std::runtime_error("Could not find android HashMap.put");
				}
				env->SetObjectField(loginOptions, clientId, options.clientId.toJavaString(env));
				env->SetObjectField(loginOptions, redirectURL, options.redirectURL.toJavaString(env));
				env->SetObjectField(loginOptions, scopes, options.scopes.toJavaObjectArray(env, stringClass, [](auto env, auto& scope) {
					return scope.toJavaString(env);
				}));
				env->SetObjectField(loginOptions, tokenSwapURL, options.tokenSwapURL.toJavaString(env));
				env->SetObjectField(loginOptions, tokenRefreshURL, options.tokenRefreshURL.toJavaString(env));
				jobject paramsMap = env->NewObject(hashMapClass, hashMapConstructor);
				for(auto& pair : options.params) {
					env->CallObjectMethod(paramsMap, hashMapPut,
						pair.first.toJavaString(env), pair.second.toJavaString(env));
				}
				env->SetObjectField(loginOptions, params, paramsMap);
				return loginOptions;
			}
		}

		namespace SpotifyAuthActivity {
			jclass javaClass = nullptr;
			jmethodID performAuthFlow = nullptr;
			jmethodID finish = nullptr;

			inline std::vector<jmethodID> methods() {
				return { performAuthFlow, finish };
			}
		}

		namespace SpotifyNativeAuthActivityListener {
			struct Params {
				std::function<void(JNIEnv*,jobject,sh::SpotifySession)> onReceiveSession;
				std::function<void(JNIEnv*,jobject,String)> onReceiveCode;
				std::function<void(JNIEnv*,jobject)> onCancel;
				std::function<void(JNIEnv*,jobject,String)> onFailure;
			};

			jclass javaClass = nullptr;
			jmethodID instantiate = nullptr;

			inline std::vector<jmethodID> methods() {
				return { instantiate };
			}

			inline jobject newInstance(JNIEnv* env, Params params) {
				return env->NewObject(javaClass, instantiate,
					(jlong)(new std::function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						jobject activity = args[0];
						jobject session = args[1];
						jstring refreshToken = (jstring)env->GetObjectField(session, android::SpotifySession::refreshToken);
						jobjectArray scopesArray = (jobjectArray)env->GetObjectField(session, android::SpotifySession::scopes);
						auto scopes = ArrayList<String>(env, scopesArray, [](JNIEnv* env, jobject scope) {
							return String(env, (jstring)scope);
						});
						params.onReceiveSession(env, activity, sh::SpotifySession(
							String(env, (jstring)env->GetObjectField(session, android::SpotifySession::accessToken)),
							std::chrono::system_clock::from_time_t((time_t)(env->GetLongField(session, android::SpotifySession::expireTime) / 1000)),
							(refreshToken != nullptr) ? String(env, refreshToken) : String(),
							scopes
						));
					})),
					(jlong)(new std::function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						jobject activity = args[0];
						jstring code = (jstring)args[1];
						params.onReceiveCode(env, activity, String(env, code));
					})),
					(jlong)(new std::function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						jobject activity = args[0];
						params.onCancel(env, activity);
					})),
					(jlong)(new std::function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						jobject activity = args[0];
						jstring error = (jstring)args[1];
						params.onReceiveCode(env, activity, String(env, error));
					}))
				);
			}
		}
	}




	Promise<Optional<SpotifySession>> SpotifyAuth::authenticate(Options options) {
		JavaVM* vm = getMainJavaVM();
		if(vm == nullptr) {
			throw std::runtime_error("Java VM has not been initialized");
		}
		ScopedJNIEnv scopedEnv(vm);
		JNIEnv* env = scopedEnv.getEnv();
		jobject context = getAndroidAppContext(env);
		if(context == nullptr) {
			throw std::runtime_error("SoundHole.mainActivity has not been set");
		}
		return Promise<Optional<SpotifySession>>([=](auto resolve, auto reject) {
			env->CallStaticVoidMethod(android::SpotifyAuthActivity::javaClass, android::SpotifyAuthActivity::performAuthFlow,
				context, android::SpotifyLoginOptions::from(env, options),
				android::SpotifyNativeAuthActivityListener::newInstance(env, {
					.onReceiveSession = [=](JNIEnv* env, jobject activity, sh::SpotifySession session) {
						env->CallVoidMethod(activity, android::SpotifyAuthActivity::finish,
							newAndroidFunction(env, [=](JNIEnv* env, std::vector<jobject> args) {
								resolve(session);
							}));
					},
					.onReceiveCode = [=](JNIEnv* env, jobject activity, String code) {
						sh::SpotifyAuth::swapCodeForToken(code, options.tokenSwapURL).then([=](SpotifySession session) {
							ScopedJNIEnv scopedEnv(getMainJavaVM());
							JNIEnv* env = scopedEnv.getEnv();
							env->CallVoidMethod(activity, android::SpotifyAuthActivity::finish,
								newAndroidFunction(env, [=](JNIEnv* env, std::vector<jobject> args) {
									resolve(session);
								}));
						}).except([=](std::exception_ptr error) {
							ScopedJNIEnv scopedEnv(getMainJavaVM());
							JNIEnv* env = scopedEnv.getEnv();
							env->CallVoidMethod(activity, android::SpotifyAuthActivity::finish,
								newAndroidFunction(env, [=](JNIEnv* env, std::vector<jobject> args) {
									reject(error);
								}));
						});
					},
					.onCancel = [=](JNIEnv* env, jobject activity) {
						env->CallVoidMethod(activity, android::SpotifyAuthActivity::finish,
							newAndroidFunction(env, [=](JNIEnv* env, std::vector<jobject> args) {
								resolve(std::nullopt);
							}));
					},
					.onFailure = [=](JNIEnv* env, jobject activity, String error) {
						env->CallVoidMethod(activity, android::SpotifyAuthActivity::finish,
							newAndroidFunction(env, [=](JNIEnv* env, std::vector<jobject> args) {
								reject(SpotifyError(SpotifyError::Code::SDK_FAILED, error));
							}));
					}
				}));
		});
	}
}




extern "C" JNIEXPORT void JNICALL
Java_com_lufinkey_soundholecore_SpotifyUtils_initAuthUtils(JNIEnv* env, jclass utilsClass, jclass sessionClass, jclass loginOptions, jclass authActivityClass, jclass nativeAuthActivityListener) {
	using namespace sh;

	android::SpotifySession::javaClass = (jclass)env->NewGlobalRef(sessionClass);
	android::SpotifySession::accessToken = env->GetFieldID(sessionClass, "accessToken", "Ljava/lang/String;");
	android::SpotifySession::expireTime = env->GetFieldID(sessionClass, "expireTime", "J");
	android::SpotifySession::refreshToken = env->GetFieldID(sessionClass, "refreshToken", "Ljava/lang/String;");
	android::SpotifySession::scopes = env->GetFieldID(sessionClass, "scopes", "[Ljava/lang/String;");

	android::SpotifyLoginOptions::javaClass = (jclass)env->NewGlobalRef(loginOptions);
	android::SpotifyLoginOptions::instantiate = env->GetMethodID(loginOptions, "<init>", "()V");
	android::SpotifyLoginOptions::clientId = env->GetFieldID(loginOptions, "clientId", "Ljava/lang/String;");
	android::SpotifyLoginOptions::redirectURL = env->GetFieldID(loginOptions, "redirectURL", "Ljava/lang/String;");
	android::SpotifyLoginOptions::scopes = env->GetFieldID(loginOptions, "scopes", "[Ljava/lang/String;");
	android::SpotifyLoginOptions::tokenSwapURL = env->GetFieldID(loginOptions, "tokenSwapURL", "Ljava/lang/String;");
	android::SpotifyLoginOptions::tokenRefreshURL = env->GetFieldID(loginOptions, "tokenRefreshURL", "Ljava/lang/String;");
	android::SpotifyLoginOptions::params = env->GetFieldID(loginOptions, "params", "Ljava/util/HashMap;");

	android::SpotifyAuthActivity::javaClass = (jclass)env->NewGlobalRef(authActivityClass);
	android::SpotifyAuthActivity::performAuthFlow = env->GetStaticMethodID(authActivityClass, "performAuthFlow",
		"(Landroid/content/Context;Lcom/lufinkey/soundholecore/SpotifyLoginOptions;Lcom/lufinkey/soundholecore/SpotifyAuthActivityListener;)V");
	android::SpotifyAuthActivity::finish = env->GetMethodID(authActivityClass, "finish", "(Lcom/lufinkey/soundholecore/NativeFunction;)V");

	android::SpotifyNativeAuthActivityListener::javaClass = (jclass)env->NewGlobalRef(nativeAuthActivityListener);
	android::SpotifyNativeAuthActivityListener::instantiate = env->GetMethodID(nativeAuthActivityListener, "<init>", "(JJJJ)V");

	for(auto& fieldList : { android::SpotifySession::fields(), android::SpotifyLoginOptions::fields() }) {
		for(auto field : fieldList) {
			if(field == nullptr) {
				throw std::runtime_error("missing java field for Spotify");
			}
		}
	}

	for(auto& methodList : { android::SpotifyLoginOptions::methods(), android::SpotifyAuthActivity::methods(), android::SpotifyNativeAuthActivityListener::methods() }) {
		for(auto method : methodList) {
			if(method == nullptr) {
				throw std::runtime_error("missing java method for Spotify");
			}
		}
	}
}

#endif
#endif

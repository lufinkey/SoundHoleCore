//
//  AndroidUtils.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/4/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include <jni.h>
#include "AndroidUtils.hpp"
#include <functional>

namespace sh {
	JavaVM* mainJavaVM = nullptr;

	JavaVM* getMainJavaVM() {
		return mainJavaVM;
	}



	jclass AndroidFunctionClass = nullptr;
	std::string AndroidFunctionClassName = "com/lufinkey/soundholecore/NativeFunction";
	jclass getAndroidFunctionClass(JNIEnv* env) {
		if(AndroidFunctionClass != nullptr) {
			return AndroidFunctionClass;
		}
		AndroidFunctionClass = env->FindClass(AndroidFunctionClassName.c_str());
		if(AndroidFunctionClass == nullptr) {
			throw std::runtime_error("Could not find class named " + AndroidFunctionClassName);
		}
		return AndroidFunctionClass;
	}

	jobject newAndroidFunction(JNIEnv* env, std::function<void(JNIEnv*,jobject)> func) {
		jclass callbackClass = getAndroidFunctionClass(env);
		jmethodID methodId = env->GetMethodID(callbackClass, "<init>", "(J)V");
		return env->NewObject(
				callbackClass,
				methodId,
				(jlong)(new std::function<void(JNIEnv*,jobject)>(func)));
	}



	jclass AndroidCallbackClass = nullptr;
	std::string AndroidCallbackClassName = "com/lufinkey/soundholecore/NativeCallback";
	jclass getAndroidCallbackClass(JNIEnv* env) {
		if(AndroidCallbackClass != nullptr) {
			return AndroidCallbackClass;
		}
		AndroidCallbackClass = env->FindClass(AndroidCallbackClassName.c_str());
		if(AndroidCallbackClass == nullptr) {
			throw std::runtime_error("Could not find class named " + AndroidCallbackClassName);
		}
		return AndroidCallbackClass;
	}

	jobject newAndroidCallback(JNIEnv* env, std::function<void(JNIEnv*,jobject)> onResolve, std::function<void(JNIEnv*,jobject)> onReject) {
		jclass callbackClass = getAndroidCallbackClass(env);
		jmethodID methodId = env->GetMethodID(callbackClass, "<init>", "(JJ)V");
		return env->NewObject(
				callbackClass,
				methodId,
				(jlong)(new std::function<void(JNIEnv*,jobject)>(onResolve)),
				(jlong)(new std::function<void(JNIEnv*,jobject)>(onReject)));
	}



	jclass AndroidSpotifyPlayerInitCallbackClass = nullptr;
	std::string AndroidSpotifyPlayerInitCallbackClassName = "com/lufinkey/soundholecore/NativeSpotifyPlayerInitCallback";
	jclass getAndroidSpotifyPlayerInitCallbackClass(JNIEnv* env) {
		if(AndroidSpotifyPlayerInitCallbackClass != nullptr) {
			return AndroidSpotifyPlayerInitCallbackClass;
		}
		AndroidSpotifyPlayerInitCallbackClass = env->FindClass(AndroidSpotifyPlayerInitCallbackClassName.c_str());
		if(AndroidSpotifyPlayerInitCallbackClass == nullptr) {
			throw std::runtime_error("Could not find class named " + AndroidSpotifyPlayerInitCallbackClassName);
		}
		return AndroidSpotifyPlayerInitCallbackClass;
	}

	jobject newAndroidSpotifyPlayerInitCallback(JNIEnv* env, std::function<void(JNIEnv*,jobject)> onResolve, std::function<void(JNIEnv*,jobject)> onReject) {
		jclass callbackClass = getAndroidSpotifyPlayerInitCallbackClass(env);
		jmethodID methodId = env->GetMethodID(callbackClass, "<init>", "(JJ)V");
		return env->NewObject(
				callbackClass,
				methodId,
				(jlong)(new std::function<void(JNIEnv*,jobject)>(onResolve)),
				(jlong)(new std::function<void(JNIEnv*,jobject)>(onReject)));
	}



	jclass AndroidSpotifyPlayerOpCallbackClass = nullptr;
	std::string AndroidSpotifyPlayerOpCallbackClassName = "com/lufinkey/soundholecore/NativeSpotifyPlayerOpCallback";
	jclass getAndroidSpotifyPlayerOpCallbackClass(JNIEnv* env) {
		if(AndroidSpotifyPlayerOpCallbackClass != nullptr) {
			return AndroidSpotifyPlayerOpCallbackClass;
		}
		AndroidSpotifyPlayerOpCallbackClass = env->FindClass(AndroidSpotifyPlayerOpCallbackClassName.c_str());
		if(AndroidSpotifyPlayerOpCallbackClass == nullptr) {
			throw std::runtime_error("Could not find class named " + AndroidSpotifyPlayerOpCallbackClassName);
		}
		return AndroidSpotifyPlayerOpCallbackClass;
	}

	jobject newAndroidSpotifyPlayerOpCallback(JNIEnv* env, std::function<void(JNIEnv*)> onResolve, std::function<void(JNIEnv*,jobject)> onReject) {
		jclass callbackClass = getAndroidSpotifyPlayerInitCallbackClass(env);
		jmethodID methodId = env->GetMethodID(callbackClass, "<init>", "(JJ)V");
		return env->NewObject(
			callbackClass,
			methodId,
			(jlong)(new std::function<void(JNIEnv*,jobject)>([=](JNIEnv* env,jobject) { onResolve(env); })),
			(jlong)(new std::function<void(JNIEnv*,jobject)>(onReject)));
	}



	jclass AndroidSpotifyPlayerEventHandlerClass = nullptr;
	std::string AndroidSpotifyPlayerEventHandlerClassName = "com/lufinkey/soundholecore/SpotifyPlayerEventHandler";
	jclass getAndroidSpotifyPlayerEventHandlerClass(JNIEnv* env) {
		if(AndroidSpotifyPlayerEventHandlerClass != nullptr) {
			return AndroidSpotifyPlayerEventHandlerClass;
		}
		AndroidSpotifyPlayerEventHandlerClass = env->FindClass(AndroidSpotifyPlayerEventHandlerClassName.c_str());
		if(AndroidSpotifyPlayerEventHandlerClass == nullptr) {
			throw std::runtime_error("Could not find class named " + AndroidSpotifyPlayerEventHandlerClassName);
		}
		return AndroidSpotifyPlayerEventHandlerClass;
	}

	jobject newAndroidSpotifyPlayerEventHandler(JNIEnv* env, SpotifyPlayerEventHandlerParams params) {
		std::function<void(JNIEnv*)> onLoggedIn = params.onLoggedIn;
		std::function<void(JNIEnv*)> onLoggedOut = params.onLoggedOut;
		std::function<void(JNIEnv*,jobject)> onLoginFailed = params.onLoginFailed;
		std::function<void(JNIEnv*)> onTemporaryError = params.onTemporaryError;
		std::function<void(JNIEnv*,jstring)> onConnectionMessage = params.onConnectionMessage;
		std::function<void(JNIEnv*,jobject)> onPlaybackEvent = params.onPlaybackEvent;
		std::function<void(JNIEnv*,jobject)> onPlaybackError = params.onPlaybackError;

		jclass handlerClass = getAndroidSpotifyPlayerEventHandlerClass(env);
		jmethodID methodId = env->GetMethodID(handlerClass, "<init>", "(JJJJJJJ)V");
		return env->NewObject(
			handlerClass,
			methodId,
			(jlong)(new std::function<void(JNIEnv*,jobject)>([=](JNIEnv* env, jobject){
				onLoggedIn(env);
			})),
			(jlong)(new std::function<void(JNIEnv*,jobject)>([=](JNIEnv* env, jobject){
				onLoggedOut(env);
			})),
			(jlong)(new std::function<void(JNIEnv*,jobject)>(onLoginFailed)),
			(jlong)(new std::function<void(JNIEnv*,jobject)>([=](JNIEnv* env, jobject){
				onTemporaryError(env);
			})),
			(jlong)(new std::function<void(JNIEnv*,jobject)>([=](JNIEnv* env, jobject obj) { onConnectionMessage(env, (jstring)obj); })),
			(jlong)(new std::function<void(JNIEnv*,jobject)>(onPlaybackEvent)),
			(jlong)(new std::function<void(JNIEnv*,jobject)>(onPlaybackError))
		);
	}
}




extern "C" JNIEXPORT void JNICALL
Java_com_lufinkey_soundholecore_SoundHole_init(JNIEnv* env, jclass javaClass) {
	auto vmResult = env->GetJavaVM(&sh::mainJavaVM);
	if(vmResult != 0 || sh::mainJavaVM == nullptr) {
		throw std::runtime_error("Could not get java VM");
	}
}

extern "C" JNIEXPORT void JNICALL
Java_com_lufinkey_soundholecore_NativeFunction_callFunction(JNIEnv* env, jobject, jlong funcPtr, jobject arg) {
	std::function<void(JNIEnv*,jobject)> func = *((std::function<void(JNIEnv*,jobject)>*)funcPtr);
	func(env,arg);
}

extern "C" JNIEXPORT void JNICALL
Java_com_lufinkey_soundholecore_NativeFunction_destroyFunction(JNIEnv* env, jobject, jlong funcPtr) {
	delete ((std::function<void(JNIEnv*,jobject)>*)funcPtr);
}

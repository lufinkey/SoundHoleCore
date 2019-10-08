//
//  AndroidUtils.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/4/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#ifdef __ANDROID__

#include <jni.h>
#include "AndroidUtils.hpp"
#include <functional>

namespace sh {
	namespace android {
		namespace SoundHole {
			jclass javaClass;
			jmethodID getAppContext;
		}
	}


	JavaVM* mainJavaVM = nullptr;

	JavaVM* getMainJavaVM() {
		return mainJavaVM;
	}


	jobject getAndroidAppContext(JNIEnv* env) {
		return env->CallStaticObjectMethod(android::SoundHole::javaClass, android::SoundHole::getAppContext);
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

	jobject newAndroidFunction(JNIEnv* env, std::function<void(JNIEnv*,std::vector<jobject>)> func) {
		jclass callbackClass = getAndroidFunctionClass(env);
		jmethodID methodId = env->GetMethodID(callbackClass, "<init>", "(J)V");
		return env->NewObject(
			callbackClass,
			methodId,
			(jlong)(new std::function<void(JNIEnv*,std::vector<jobject>)>(func)));
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
			(jlong)(new std::function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onResolve(env, args[0]);
			})),
			(jlong)(new std::function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onReject(env, args[0]);
			})));
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
			(jlong)(new std::function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onResolve(env, args[0]);
			})),
			(jlong)(new std::function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onReject(env, args[0]);
			})));
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
			(jlong)(new std::function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onResolve(env);
			})),
			(jlong)(new std::function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onReject(env, args[0]);
			})));
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

	jobject newAndroidSpotifyPlayerEventHandler(JNIEnv* env, jobject player, SpotifyPlayerEventHandlerParams params) {
		auto onLoggedIn = params.onLoggedIn;
		auto onLoggedOut = params.onLoggedOut;
		auto onLoginFailed = params.onLoginFailed;
		auto onTemporaryError = params.onTemporaryError;
		auto onConnectionMessage = params.onConnectionMessage;
		auto onDisconnect = params.onDisconnect;
		auto onReconnect = params.onReconnect;
		auto onPlaybackEvent = params.onPlaybackEvent;
		auto onPlaybackError = params.onPlaybackError;

		jclass handlerClass = getAndroidSpotifyPlayerEventHandlerClass(env);
		jmethodID methodId = env->GetMethodID(handlerClass, "<init>", "(Lcom/spotify/sdk/android/player/SpotifyPlayer;JJJJJJJJJ)V");
		return env->NewObject(
			handlerClass,
			methodId,
			(jlong)(new std::function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onLoggedIn(env);
			})),
			(jlong)(new std::function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onLoggedOut(env);
			})),
			(jlong)(new std::function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onLoginFailed(env, args[0]);
			})),
			(jlong)(new std::function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onTemporaryError(env);
			})),
			(jlong)(new std::function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onConnectionMessage(env, (jstring)args[0]);
			})),
			(jlong)(new std::function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onDisconnect(env);
			})),
			(jlong)(new std::function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onReconnect(env);
			})),
			(jlong)(new std::function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onPlaybackEvent(env, args[0]);
			})),
			(jlong)(new std::function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
				onPlaybackError(env, args[0]);
			}))
		);
	}

	void destroyAndroidSpotifyPlayerEventHandler(JNIEnv* env, jobject eventHandler) {
		jclass javaClass = env->GetObjectClass(eventHandler);
		jmethodID methodId = env->GetMethodID(javaClass, "destroy", "()V");
		env->CallVoidMethod(eventHandler, methodId);
	}
}




extern "C" JNIEXPORT void JNICALL
Java_com_lufinkey_soundholecore_SoundHole_staticInit(JNIEnv* env, jclass javaClass) {
	using namespace sh;
	auto vmResult = env->GetJavaVM(&sh::mainJavaVM);
	if(vmResult != 0 || sh::mainJavaVM == nullptr) {
		throw std::runtime_error("Could not get java VM");
	}

	// initialize NodeJSEmbed
	jclass nodejsClass = env->FindClass("com/lufinkey/embed/NodeJS");
	if(nodejsClass == nullptr) {
		throw std::runtime_error("Could not load com/lufinkey/embed/NodeJS");
	}

	android::SoundHole::javaClass = (jclass)env->NewGlobalRef(javaClass);
	android::SoundHole::getAppContext = env->GetStaticMethodID(javaClass, "getAppContext", "()Landroid/content/Context;");
}

extern "C" JNIEXPORT void JNICALL
Java_com_lufinkey_soundholecore_NativeFunction_callFunction(JNIEnv* env, jobject, jlong funcPtr, jobjectArray argsArray) {
	std::function<void(JNIEnv*,std::vector<jobject>)> func = *((std::function<void(JNIEnv*,std::vector<jobject>)>*)funcPtr);
	std::vector<jobject> args;
	jsize argsSize = env->GetArrayLength(argsArray);
	args.reserve((size_t)argsSize);
	for(jsize i=0; i<argsSize; i++) {
		args.push_back(env->GetObjectArrayElement(argsArray, i));
	}
	func(env,args);
}

extern "C" JNIEXPORT void JNICALL
Java_com_lufinkey_soundholecore_NativeFunction_destroyFunction(JNIEnv* env, jobject, jlong funcPtr) {
	delete ((std::function<void(JNIEnv*,std::vector<jobject>)>*)funcPtr);
}

#endif

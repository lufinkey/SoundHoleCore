//
//  AndroidUtils.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/4/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#ifdef __ANDROID__

#include <functional>
#include <soundhole/common.hpp>
#include <jni.h>

namespace sh {
	JavaVM* getMainJavaVM();
	jobject getAndroidMainActivity(JNIEnv* env);
	jobject getAndroidAppContext(JNIEnv* env);

	jobject newAndroidFunction(JNIEnv* env, std::function<void(JNIEnv*,std::vector<jobject>)> func);
	jobject newAndroidCallback(JNIEnv* env, std::function<void(JNIEnv*,jobject)> onResolve, std::function<void(JNIEnv*,jobject)> onReject);
	jobject newAndroidSpotifyPlayerInitCallback(JNIEnv* env, std::function<void(JNIEnv*,jobject)> onResolve, std::function<void(JNIEnv*,jobject)> onReject);
	jobject newAndroidSpotifyPlayerOpCallback(JNIEnv* env, std::function<void(JNIEnv*)> onResolve, std::function<void(JNIEnv*,jobject)> onReject);

	struct SpotifyPlayerEventHandlerParams {
		std::function<void(JNIEnv*)> onLoggedIn;
		std::function<void(JNIEnv*)> onLoggedOut;
		std::function<void(JNIEnv*,jobject)> onLoginFailed;
		std::function<void(JNIEnv*)> onTemporaryError;
		std::function<void(JNIEnv*,jstring)> onConnectionMessage;
		std::function<void(JNIEnv*)> onDisconnect;
		std::function<void(JNIEnv*)> onReconnect;
		std::function<void(JNIEnv*,jobject)> onPlaybackEvent;
		std::function<void(JNIEnv*,jobject)> onPlaybackError;
	};
	jobject newAndroidSpotifyPlayerEventHandler(JNIEnv* env, jobject player, SpotifyPlayerEventHandlerParams params);
	void destroyAndroidSpotifyPlayerEventHandler(JNIEnv* env, jobject eventHandler);
}

#endif

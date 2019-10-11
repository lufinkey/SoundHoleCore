//
//  AndroidUtils.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/4/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#ifdef __ANDROID__

#include <jni.h>
#include <functional>
#include <soundhole/common.hpp>
#include <soundhole/providers/spotify/api/SpotifyAuth.hpp>
#include <soundhole/providers/spotify/api/SpotifyError.hpp>
#include <embed/nodejs/NodeJS_jni.hpp>

#ifdef TARGETPLATFORM_ANDROID

namespace sh {
	using ScopedJNIEnv = embed::nodejs::ScopedJNIEnv;

	JavaVM* getMainJavaVM();

	namespace android {
		namespace SoundHole {
			extern jclass javaClass;
			jobject getAppContext(JNIEnv* env);
			void runOnMainThread(JNIEnv* env, Function<void(JNIEnv*)> func);
		}

		namespace NativeFunction {
			extern jclass javaClass;
			jobject newObject(JNIEnv* env, Function<void(JNIEnv*,std::vector<jobject>)> func);
		}

		namespace NativeCallback {
			extern jclass javaClass;
			jobject newObject(JNIEnv* env, Function<void(JNIEnv*,jobject)> onResolve, Function<void(JNIEnv*,jobject)> onReject);
		}

		namespace NativeSpotifyPlayerInitCallback {
			extern jclass javaClass;
			jobject newObject(JNIEnv* env, Function<void(JNIEnv*,jobject)> onResolve, Function<void(JNIEnv*,jobject)> onReject);
		}

		namespace NativeSpotifyPlayerOpCallback {
			extern jclass javaClass;
			jobject newObject(JNIEnv* env, Function<void(JNIEnv*)> onResolve, Function<void(JNIEnv*,jobject)> onReject);
		}

		namespace SpotifyPlayerEventHandler {
			extern jclass javaClass;
			struct Params {
				Function<void(JNIEnv*)> onLoggedIn;
				Function<void(JNIEnv*)> onLoggedOut;
				Function<void(JNIEnv*,jobject)> onLoginFailed;
				Function<void(JNIEnv*)> onTemporaryError;
				Function<void(JNIEnv*,jstring)> onConnectionMessage;
				Function<void(JNIEnv*)> onDisconnect;
				Function<void(JNIEnv*)> onReconnect;
				Function<void(JNIEnv*,jobject)> onPlaybackEvent;
				Function<void(JNIEnv*,jobject)> onPlaybackError;
			};
			jobject newObject(JNIEnv* env, jobject player, Params params);
			void destroy(JNIEnv* env, jobject self);
		}

		namespace SpotifySession {
			extern jclass javaClass;
			jstring getAccessToken(JNIEnv* env, jobject self);
			jlong getExpireTime(JNIEnv* env, jobject self);
			jstring getRefreshToken(JNIEnv* env, jobject self);
			jobjectArray getScopes(JNIEnv* env, jobject self);
		}

		namespace SpotifyLoginOptions {
			extern jclass javaClass;
			jobject newObject(JNIEnv* env);
			jobject from(JNIEnv* env, sh::SpotifyAuth::Options options);
			void setClientId(JNIEnv* env, jobject self, jstring clientId);
			void setRedirectURL(JNIEnv* env, jobject self, jstring redirectURL);
			void setScopes(JNIEnv* env, jobject self, jobjectArray scopes);
			void setTokenSwapURL(JNIEnv* env, jobject self, jstring tokenSwapURL);
			void setTokenRefreshURL(JNIEnv* env, jobject self, jstring tokenRefreshURL);
			void setParams(JNIEnv* env, jobject self, jobject params);
		}

		namespace SpotifyAuthActivity {
			extern jclass javaClass;
			void performAuthFlow(JNIEnv* env, jobject context, jobject loginOptions, jobject listener);
			void finish(JNIEnv* env, jobject self, jobject completion);
		}

		namespace SpotifyNativeAuthActivityListener {
			extern jclass javaClass;
			struct Params {
				Function<void(JNIEnv*,jobject,sh::SpotifySession)> onReceiveSession;
				Function<void(JNIEnv*,jobject,String)> onReceiveCode;
				Function<void(JNIEnv*,jobject)> onCancel;
				Function<void(JNIEnv*,jobject,String)> onFailure;
			};
			jobject newObject(JNIEnv* env, Params params);
		}

		namespace SpotifyError {
			extern jclass javaClass;

			extern std::map<int,std::tuple<sh::SpotifyError::Code,String>> enumMap;
			void generateEnumMap(JNIEnv* env);

			jobject getEnum(JNIEnv* env, const char* name);

			jint getNativeCode(JNIEnv* env, jobject self);
		}

		namespace SpotifyUtils {
			extern jclass javaClass;
			jobject newObject(JNIEnv* env);
			void getPlayer(JNIEnv* env, jobject self, jstring clientId, jstring accessToken, jobject callback);
			void destroyPlayer(JNIEnv* env, jobject self, jobject player);
		}

		namespace SpotifyPlayer {
			extern jclass javaClass;
			extern jmethodID _playUri;
			extern jmethodID _queue;
			extern jmethodID _seekToPosition;
			extern jmethodID _pause;
			extern jmethodID _resume;
			extern jmethodID _getPlaybackState;
			extern jmethodID _getMetadata;
			extern jmethodID _skipToNext;
			extern jmethodID _skipToPrevious;
			extern jmethodID _setShuffle;
			extern jmethodID _setRepeat;
			jboolean isLoggedIn(JNIEnv* env, jobject self);
			jboolean login(JNIEnv* env, jobject self, jstring oauthToken);
			jboolean logout(JNIEnv* env, jobject self);
		}

		namespace SpotifyPlaybackState {
			extern jclass javaClass;
			extern jfieldID _isPlaying;
			extern jfieldID _isRepeating;
			extern jfieldID _isShuffling;
			extern jfieldID _isActiveDevice;
			extern jfieldID _positionMs;
		}

		namespace SpotifyMetadata {
			extern jclass javaClass;
			extern jfieldID _contextName;
			extern jfieldID _contextUri;
			extern jfieldID _prevTrack;
			extern jfieldID _currentTrack;
			extern jfieldID _nextTrack;
		}

		namespace SpotifyTrack {
			extern jclass javaClass;
			extern jfieldID _name;
			extern jfieldID _uri;
			extern jfieldID _artistName;
			extern jfieldID _artistUri;
			extern jfieldID _albumName;
			extern jfieldID _albumUri;
			extern jfieldID _durationMs;
			extern jfieldID _indexInContext;
			extern jfieldID _albumCoverWebUrl;
		}
	}
}

#endif
#endif

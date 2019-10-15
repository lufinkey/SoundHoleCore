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

#ifdef TARGETPLATFORM_ANDROID

namespace sh {
	JavaVM* mainJavaVM = nullptr;

	JavaVM* getMainJavaVM() {
		return mainJavaVM;
	}

	namespace android {
		namespace SoundHole {
			jclass javaClass = nullptr;
		}


		namespace Utils {
			jclass javaClass = nullptr;
			jfieldID _appContext = nullptr;
			jmethodID _runOnMainThread = nullptr;

			inline std::vector<jfieldID> fields() {
				return { _appContext };
			}

			inline std::vector<jmethodID> methods() {
				return { _runOnMainThread };
			}

			jobject getAppContext(JNIEnv* env) {
				return env->GetStaticObjectField(javaClass, _appContext);
			}

			void runOnMainThread(JNIEnv* env, Function<void(JNIEnv*)> func) {
				env->CallStaticVoidMethod(javaClass, _runOnMainThread,
					android::NativeFunction::newObject(env, [=](auto env, auto args) {
						func(env);
					}));
			}
		}


		namespace NativeFunction {
			jclass javaClass = nullptr;
			jmethodID _instantiate = nullptr;

			inline std::vector<jmethodID> methods() {
				return { _instantiate };
			}

			jobject newObject(JNIEnv* env, Function<void(JNIEnv*,std::vector<jobject>)> func) {
				return env->NewObject(javaClass, _instantiate,
					(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>(func)));
			}
		}


		namespace NativeCallback {
			jclass javaClass = nullptr;
			jmethodID _instantiate = nullptr;

			inline std::vector<jmethodID> methods() {
				return { _instantiate };
			}

			jobject newObject(JNIEnv* env, Function<void(JNIEnv*,jobject)> onResolve, Function<void(JNIEnv*,jobject)> onReject) {
				return env->NewObject(javaClass, _instantiate,
					(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						onResolve(env, args[0]);
					})),
					(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						onReject(env, args[0]);
					})));
			}
		}


		namespace NativeSpotifyPlayerInitCallback {
			jclass javaClass = nullptr;
			jmethodID _instantiate = nullptr;

			inline std::vector<jmethodID> methods() {
				return { _instantiate };
			}

			jobject newObject(JNIEnv* env, Function<void(JNIEnv*,jobject)> onResolve, Function<void(JNIEnv*,jobject)> onReject) {
				return env->NewObject(javaClass, _instantiate,
					(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						onResolve(env, args[0]);
					})),
					(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						onReject(env, args[0]);
					})));
			}
		}


		namespace NativeSpotifyPlayerOpCallback {
			jclass javaClass = nullptr;
			jmethodID _instantiate = nullptr;

			inline std::vector<jmethodID> methods() {
				return { _instantiate };
			}

			jobject newObject(JNIEnv* env, Function<void(JNIEnv*)> onResolve, Function<void(JNIEnv*,jobject)> onReject) {
				return env->NewObject(javaClass, _instantiate,
					(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						onResolve(env);
					})),
					(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						onReject(env, args[0]);
					})));
			}
		}


		namespace SpotifyPlayerEventHandler {
			jclass javaClass = nullptr;
			jmethodID _instantiate = nullptr;
			jmethodID _destroy = nullptr;

			inline std::vector<jmethodID> methods() {
				return { _instantiate, _destroy };
			}

			jobject newObject(JNIEnv* env, jobject player, Params params) {
				auto onLoggedIn = params.onLoggedIn;
				auto onLoggedOut = params.onLoggedOut;
				auto onLoginFailed = params.onLoginFailed;
				auto onTemporaryError = params.onTemporaryError;
				auto onConnectionMessage = params.onConnectionMessage;
				auto onDisconnect = params.onDisconnect;
				auto onReconnect = params.onReconnect;
				auto onPlaybackEvent = params.onPlaybackEvent;
				auto onPlaybackError = params.onPlaybackError;
				return env->NewObject(
					javaClass, _instantiate,
					player,
					(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						onLoggedIn(env);
					})),
					(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						onLoggedOut(env);
					})),
					(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						onLoginFailed(env, args[0]);
					})),
					(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						onTemporaryError(env);
					})),
					(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						onConnectionMessage(env, (jstring)args[0]);
					})),
					(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						onDisconnect(env);
					})),
					(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						onReconnect(env);
					})),
					(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						onPlaybackEvent(env, args[0]);
					})),
					(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						onPlaybackError(env, args[0]);
					}))
				);
			}

			void destroy(JNIEnv* env, jobject self) {
				env->CallVoidMethod(self, _destroy);
			}
		}


		namespace SpotifySession {
			jclass javaClass = nullptr;
			jfieldID _accessToken = nullptr;
			jfieldID _expireTime = nullptr;
			jfieldID _refreshToken = nullptr;
			jfieldID _scopes = nullptr;

			inline std::vector<jfieldID> fields() {
				return { _accessToken, _expireTime, _refreshToken, _scopes };
			}

			jstring getAccessToken(JNIEnv* env, jobject self) {
				return (jstring)env->GetObjectField(self, _accessToken);
			}

			jlong getExpireTime(JNIEnv* env, jobject self) {
				return env->GetLongField(self, _expireTime);
			}

			jstring getRefreshToken(JNIEnv* env, jobject self) {
				return (jstring)env->GetObjectField(self, _refreshToken);
			}

			jobjectArray getScopes(JNIEnv* env, jobject self) {
				return (jobjectArray)env->GetObjectField(self, _scopes);
			}
		}


		namespace SpotifyLoginOptions {
			jclass javaClass = nullptr;
			jmethodID _instantiate = nullptr;
			jfieldID _clientId = nullptr;
			jfieldID _redirectURL = nullptr;
			jfieldID _scopes = nullptr;
			jfieldID _tokenSwapURL = nullptr;
			jfieldID _tokenRefreshURL = nullptr;
			jfieldID _params = nullptr;

			inline std::vector<jmethodID> methods() {
				return { _instantiate };
			}

			inline std::vector<jfieldID> fields() {
				return { _clientId, _redirectURL, _scopes, _tokenSwapURL, _tokenRefreshURL, _params };
			}

			jobject from(JNIEnv* env, sh::SpotifyAuth::Options options) {
				jobject loginOptions = env->NewObject(javaClass, _instantiate);
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
				setClientId(env, loginOptions, options.clientId.toJavaString(env));
				setRedirectURL(env, loginOptions, options.redirectURL.toJavaString(env));
				setScopes(env, loginOptions, options.scopes.toJavaObjectArray(env, stringClass, [](auto env, auto& scope) {
					return scope.toJavaString(env);
				}));
				setTokenSwapURL(env, loginOptions, options.tokenSwapURL.toJavaString(env));
				setTokenRefreshURL(env, loginOptions, options.tokenRefreshURL.toJavaString(env));
				jobject paramsMap = env->NewObject(hashMapClass, hashMapConstructor);
				for(auto& pair : options.params) {
					env->CallObjectMethod(paramsMap, hashMapPut,
										  pair.first.toJavaString(env), pair.second.toJavaString(env));
				}
				setParams(env, loginOptions, paramsMap);
				return loginOptions;
			}

			jobject newObject(JNIEnv* env) {
				return env->NewObject(javaClass, _instantiate);
			}

			void setClientId(JNIEnv* env, jobject self, jstring clientId) {
				env->SetObjectField(self, _clientId, clientId);
			}
			void setRedirectURL(JNIEnv* env, jobject self, jstring redirectURL) {
				env->SetObjectField(self, _redirectURL, redirectURL);
			}
			void setScopes(JNIEnv* env, jobject self, jobjectArray scopes) {
				env->SetObjectField(self, _scopes, scopes);
			}
			void setTokenSwapURL(JNIEnv* env, jobject self, jstring tokenSwapURL) {
				env->SetObjectField(self, _tokenSwapURL, tokenSwapURL);
			}
			void setTokenRefreshURL(JNIEnv* env, jobject self, jstring tokenRefreshURL) {
				env->SetObjectField(self, _tokenRefreshURL, tokenRefreshURL);
			}
			void setParams(JNIEnv* env, jobject self, jobject params) {
				env->SetObjectField(self, _params, params);
			}
		}


		namespace SpotifyAuthActivity {
			jclass javaClass = nullptr;
			jmethodID _performAuthFlow = nullptr;
			jmethodID _finish = nullptr;
			jmethodID _showProgressDialog = nullptr;
			jmethodID _hideProgressDialog = nullptr;

			inline std::vector<jmethodID> methods() {
				return { _performAuthFlow, _finish, _showProgressDialog, _hideProgressDialog };
			}

			void performAuthFlow(JNIEnv* env, jobject context, jobject loginOptions, jobject listener) {
				env->CallStaticVoidMethod(javaClass, _performAuthFlow, context, loginOptions, listener);
			}

			void finish(JNIEnv* env, jobject self, jobject completion) {
				env->CallVoidMethod(self, _finish, completion);
			}

			void showProgressDialog(JNIEnv* env, jobject self, jstring loadingText) {
				env->CallVoidMethod(self, _showProgressDialog, loadingText);
			}

			void hideProgressDialog(JNIEnv* env, jobject self) {
				env->CallVoidMethod(self, _hideProgressDialog);
			}
		}


		namespace SpotifyNativeAuthActivityListener {
			jclass javaClass = nullptr;
			jmethodID _instantiate = nullptr;

			inline std::vector<jmethodID> methods() {
				return { _instantiate };
			}

			jobject newObject(JNIEnv* env, Params params) {
				auto onReceiveSession = params.onReceiveSession;
				auto onReceiveCode = params.onReceiveCode;
				auto onCancel = params.onCancel;
				auto onFailure = params.onFailure;
				return env->NewObject(javaClass, _instantiate,
					(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						jobject activity = args[0];
						jobject session = args[1];
						jstring refreshToken = android::SpotifySession::getRefreshToken(env, session);
						jobjectArray scopesArray = android::SpotifySession::getScopes(env, session);
						auto scopes = ArrayList<String>(env, scopesArray, [](JNIEnv* env, jobject scope) {
							return String(env, (jstring)scope);
						});
						onReceiveSession(env, activity, sh::SpotifySession(
							String(env, android::SpotifySession::getAccessToken(env, session)),
							std::chrono::system_clock::from_time_t((time_t)(android::SpotifySession::getExpireTime(env, session) / 1000)),
							(refreshToken != nullptr) ? String(env, refreshToken) : String(),
							scopes
						));
					})),
					(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						jobject activity = args[0];
						jstring code = (jstring)args[1];
						onReceiveCode(env, activity, String(env, code));
					})),
					(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						jobject activity = args[0];
						onCancel(env, activity);
					})),
					(jlong)(new Function<void(JNIEnv*,std::vector<jobject>)>([=](auto env, auto args) {
						jobject activity = args[0];
						jstring error = (jstring)args[1];
						onReceiveCode(env, activity, String(env, error));
					})));
			}
		}


		namespace SpotifyError {
			jclass javaClass = nullptr;

			jfieldID _nativeCode = nullptr;

			inline std::vector<jfieldID> fields() {
				return { _nativeCode };
			}

			std::map<int,std::tuple<sh::SpotifyError::Code,String>> enumMap = {};

			void generateEnumMap(JNIEnv* env) {
				using Code = sh::SpotifyError::Code;
				auto getEntry = [=](const char* javaErrorCodeName, Code cppErrorCode, String message) -> std::pair<int,std::tuple<Code,String>> {
					jobject enumVal = getEnum(env, javaErrorCodeName);
					int javaErrorCode = (int)env->GetIntField(enumVal, _nativeCode);
					return { javaErrorCode, { cppErrorCode, message } };
				};
				enumMap = {
					getEntry("UNKNOWN", Code::SDK_FAILED, "The operation failed due to an unknown issue."),
					getEntry("kSpErrorOk", Code::SDK_FAILED, "The operation was successful. I don't know why this is an error..."),
					getEntry("kSpErrorFailed", Code::SDK_FAILED, "The operation failed due to an unspecified issue."),
					getEntry("kSpErrorInitFailed", Code::SDK_INIT_FAILED, "Audio streaming could not be initialized."),
					getEntry("kSpErrorWrongAPIVersion", Code::SDK_WRONG_API_VERSION, "Audio streaming could not be initialized because of an incompatible API version."),
					getEntry("kSpErrorNullArgument", Code::SDK_NULL_ARGUMENT, "An unexpected NULL pointer was passed as an argument to a function."),
					getEntry("kSpErrorInvalidArgument", Code::SDK_INVALID_ARGUMENT, "An unexpected argument value was passed to a function."),
					getEntry("kSpErrorUninitialized", Code::SDK_UNINITIALIZED, "Audio streaming has not yet been initialised for this application."),
					getEntry("kSpErrorAlreadyInitialized", Code::SDK_ALREADY_INITIALIZED, "Audio streaming has already been initialised for this application."),
					getEntry("kSpErrorLoginBadCredentials", Code::SDK_LOGIN_BAD_CREDENTIALS, "Login to Spotify failed because of invalid credentials."),
					getEntry("kSpErrorNeedsPremium", Code::SDK_NEEDS_PREMIUM, "This operation requires a Spotify Premium account."),
					getEntry("kSpErrorTravelRestriction", Code::SDK_TRAVEL_RESTRICTION, "Spotify user is not allowed to log in from this country."),
					getEntry("kSpErrorApplicationBanned", Code::SDK_APPLICATION_BANNED, "This application has been banned by Spotify."),
					getEntry("kSpErrorGeneralLoginError", Code::SDK_GENERAL_LOGIN_ERROR, "An unspecified login error occurred."),
					getEntry("kSpErrorUnsupported", Code::SDK_UNSUPPORTED, "The operation is not supported."),
					getEntry("kSpErrorNotActiveDevice", Code::SDK_NOT_ACTIVE_DEVICE, "The operation is not supported if the device is not the active playback device."),
					getEntry("kSpErrorAPIRateLimited", Code::SDK_API_RATE_LIMITED, "This application has made too many API requests at a time, so it is now rate-limited."),
					getEntry("kSpErrorPlaybackErrorStart", Code::SDK_PLAYBACK_START_FAILED, "Unable to start playback."),
					getEntry("kSpErrorGeneralPlaybackError", Code::SDK_GENERAL_PLAYBACK_ERROR, "An unspecified playback error occurred."),
					getEntry("kSpErrorPlaybackRateLimited", Code::SDK_PLAYBACK_RATE_LIMITED, "This application has requested track playback too many times, so it is now rate-limited."),
					getEntry("kSpErrorPlaybackCappingLimitReached", Code::SDK_PLAYBACK_CAPPING_LIMIT_REACHED, "This application's playback limit has been reached."),
					getEntry("kSpErrorAdIsPlaying", Code::SDK_AD_IS_PLAYING, "An ad is playing."),
					getEntry("kSpErrorCorruptTrack", Code::SDK_CORRUPT_TRACK, "The track is corrupted."),
					getEntry("kSpErrorContextFailed", Code::SDK_CONTEXT_FAILED, "The operation failed."),
					getEntry("kSpErrorPrefetchItemUnavailable", Code::SDK_PREFETCH_ITEM_UNAVAILABLE, "Item is unavailable for pre-fetch."),
					getEntry("kSpAlreadyPrefetching", Code::SDK_ALREADY_PREFETCHING, "Item is already pre-fetching."),
					getEntry("kSpStorageReadError", Code::SDK_STORAGE_READ_ERROR, "Storage read failed."),
					getEntry("kSpStorageWriteError", Code::SDK_STORAGE_WRITE_ERROR, "Storage write failed."),
					getEntry("kSpPrefetchDownloadFailed", Code::SDK_PREFETCH_DOWNLOAD_FAILED, "Pre-fetch download failed.")
				};
			}

			jobject getEnum(JNIEnv* env, const char* name) {
				if(javaClass == nullptr) {
					throw std::runtime_error("SpotifyError java class has not been initialized");
				}
				jfieldID fieldId = env->GetStaticFieldID(javaClass, name, "Lcom/spotify/sdk/android/player/Error;");
				return env->GetStaticObjectField(javaClass, fieldId);
			}

			jint getNativeCode(JNIEnv* env, jobject self) {
				return env->GetIntField(self, _nativeCode);
			}
		}


		namespace SpotifyUtils {
			jclass javaClass = nullptr;
			jmethodID _instantiate = nullptr;
			jmethodID _getPlayer = nullptr;
			jmethodID _destroyPlayer = nullptr;

			inline std::vector<jmethodID> methods() {
				return {
						_instantiate,
						_getPlayer, _destroyPlayer
				};
			}

			jobject newObject(JNIEnv* env) {
				return env->NewObject(javaClass, _instantiate);
			}

			void getPlayer(JNIEnv* env, jobject self, jstring clientId, jstring accessToken, jobject callback) {
				env->CallVoidMethod(self, _getPlayer, clientId, accessToken, callback);
			}

			void destroyPlayer(JNIEnv* env, jobject self, jobject player) {
				env->CallVoidMethod(self, _destroyPlayer, player);
			}
		}


		namespace SpotifyPlayer {
			jclass javaClass = nullptr;
			jmethodID _isLoggedIn = nullptr;
			jmethodID _login = nullptr;
			jmethodID _logout = nullptr;
			jmethodID _playUri = nullptr;
			jmethodID _queue = nullptr;
			jmethodID _seekToPosition = nullptr;
			jmethodID _pause = nullptr;
			jmethodID _resume = nullptr;
			jmethodID _getPlaybackState = nullptr;
			jmethodID _getMetadata = nullptr;
			jmethodID _skipToNext = nullptr;
			jmethodID _skipToPrevious = nullptr;
			jmethodID _setShuffle = nullptr;
			jmethodID _setRepeat = nullptr;

			inline std::vector<jmethodID> methods() {
				return {
						_isLoggedIn, _login, _logout,
						_playUri, _queue, _seekToPosition,
						_pause, _resume, _getPlaybackState, _getMetadata,
						_skipToNext, _skipToPrevious,
						_setShuffle, _setRepeat
				};
			}

			jboolean isLoggedIn(JNIEnv* env, jobject self) {
				return env->CallBooleanMethod(self, _isLoggedIn);
			}
			jboolean login(JNIEnv* env, jobject self, jstring oauthToken) {
				return env->CallBooleanMethod(self, _login, oauthToken);
			}
			jboolean logout(JNIEnv* env, jobject self) {
				return env->CallBooleanMethod(self, _logout);
			}
		}


		namespace SpotifyPlaybackState {
			jclass javaClass = nullptr;
			jfieldID _isPlaying = nullptr;
			jfieldID _isRepeating = nullptr;
			jfieldID _isShuffling = nullptr;
			jfieldID _isActiveDevice = nullptr;
			jfieldID _positionMs = nullptr;

			inline std::vector<jfieldID> fields() {
				return {
						_isPlaying, _isRepeating, _isShuffling, _isActiveDevice, _positionMs
				};
			}
		}


		namespace SpotifyMetadata {
			jclass javaClass = nullptr;
			jfieldID _contextName = nullptr;
			jfieldID _contextUri = nullptr;
			jfieldID _prevTrack = nullptr;
			jfieldID _currentTrack = nullptr;
			jfieldID _nextTrack = nullptr;

			inline std::vector<jfieldID> fields() {
				return {
						_contextName, _contextUri, _prevTrack, _currentTrack, _nextTrack
				};
			}
		}


		namespace SpotifyTrack {
			jclass javaClass = nullptr;
			jfieldID _name = nullptr;
			jfieldID _uri = nullptr;
			jfieldID _artistName = nullptr;
			jfieldID _artistUri = nullptr;
			jfieldID _albumName = nullptr;
			jfieldID _albumUri = nullptr;
			jfieldID _durationMs = nullptr;
			jfieldID _indexInContext = nullptr;
			jfieldID _albumCoverWebUrl = nullptr;

			inline std::vector<jfieldID> fields() {
				return {
						_name, _uri, _artistName, _artistUri, _albumName, _albumUri,
						_durationMs, _indexInContext, _albumCoverWebUrl
				};
			}
		}
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

	jclass utilsClass = env->FindClass("com/lufinkey/soundholecore/Utils");
	android::Utils::javaClass = (jclass)env->NewGlobalRef(utilsClass);
	android::Utils::_appContext = env->GetStaticFieldID(utilsClass, "appContext", "Landroid/content/Context;");
	android::Utils::_runOnMainThread = env->GetStaticMethodID(utilsClass, "runOnMainThread", "(Lcom/lufinkey/soundholecore/NativeFunction;)V");

	jclass nativeFunctionClass = env->FindClass("com/lufinkey/soundholecore/NativeFunction");
	android::NativeFunction::javaClass = (jclass)env->NewGlobalRef(nativeFunctionClass);
	android::NativeFunction::_instantiate = env->GetMethodID(nativeFunctionClass, "<init>", "(J)V");

	jclass nativeCallbackClass = env->FindClass("com/lufinkey/soundholecore/NativeCallback");
	android::NativeCallback::javaClass = (jclass)env->NewGlobalRef(nativeCallbackClass);
	android::NativeCallback::_instantiate = env->GetMethodID(nativeCallbackClass, "<init>", "(JJ)V");

	jclass nativeSpotifyPlayerInitClass = env->FindClass("com/lufinkey/soundholecore/NativeSpotifyPlayerInitCallback");
	android::NativeSpotifyPlayerInitCallback::javaClass = (jclass)env->NewGlobalRef(nativeSpotifyPlayerInitClass);
	android::NativeSpotifyPlayerInitCallback::_instantiate = env->GetMethodID(nativeSpotifyPlayerInitClass, "<init>", "(JJ)V");

	jclass nativeSpotifyPlayerOpClass = env->FindClass("com/lufinkey/soundholecore/NativeSpotifyPlayerOpCallback");
	android::NativeSpotifyPlayerOpCallback::javaClass = (jclass)env->NewGlobalRef(nativeSpotifyPlayerOpClass);
	android::NativeSpotifyPlayerOpCallback::_instantiate = env->GetMethodID(nativeSpotifyPlayerOpClass, "<init>", "(JJ)V");

	jclass spotifyPlayerEventHandlerClass = env->FindClass("com/lufinkey/soundholecore/SpotifyPlayerEventHandler");
	android::SpotifyPlayerEventHandler::javaClass = (jclass)env->NewGlobalRef(spotifyPlayerEventHandlerClass);
	android::SpotifyPlayerEventHandler::_instantiate = env->GetMethodID(spotifyPlayerEventHandlerClass, "<init>",
		"(Lcom/spotify/sdk/android/player/SpotifyPlayer;JJJJJJJJJ)V");
	android::SpotifyPlayerEventHandler::_destroy = env->GetMethodID(spotifyPlayerEventHandlerClass, "destroy", "()V");

	jclass spotifySessionClass = env->FindClass("com/lufinkey/soundholecore/SpotifySession");
	android::SpotifySession::javaClass = (jclass)env->NewGlobalRef(spotifySessionClass);
	android::SpotifySession::_accessToken = env->GetFieldID(spotifySessionClass, "accessToken", "Ljava/lang/String;");
	android::SpotifySession::_expireTime = env->GetFieldID(spotifySessionClass, "expireTime", "J");
	android::SpotifySession::_refreshToken = env->GetFieldID(spotifySessionClass, "refreshToken", "Ljava/lang/String;");
	android::SpotifySession::_scopes = env->GetFieldID(spotifySessionClass, "scopes", "[Ljava/lang/String;");

	jclass spotifyLoginOptionsClass = env->FindClass("com/lufinkey/soundholecore/SpotifyLoginOptions");
	android::SpotifyLoginOptions::javaClass = (jclass)env->NewGlobalRef(spotifyLoginOptionsClass);
	android::SpotifyLoginOptions::_instantiate = env->GetMethodID(spotifyLoginOptionsClass, "<init>", "()V");
	android::SpotifyLoginOptions::_clientId = env->GetFieldID(spotifyLoginOptionsClass, "clientId", "Ljava/lang/String;");
	android::SpotifyLoginOptions::_redirectURL = env->GetFieldID(spotifyLoginOptionsClass, "redirectURL", "Ljava/lang/String;");
	android::SpotifyLoginOptions::_scopes = env->GetFieldID(spotifyLoginOptionsClass, "scopes", "[Ljava/lang/String;");
	android::SpotifyLoginOptions::_tokenSwapURL = env->GetFieldID(spotifyLoginOptionsClass, "tokenSwapURL", "Ljava/lang/String;");
	android::SpotifyLoginOptions::_tokenRefreshURL = env->GetFieldID(spotifyLoginOptionsClass, "tokenRefreshURL", "Ljava/lang/String;");
	android::SpotifyLoginOptions::_params = env->GetFieldID(spotifyLoginOptionsClass, "params", "Ljava/util/HashMap;");

	jclass spotifyAuthActivityClass = env->FindClass("com/lufinkey/soundholecore/SpotifyAuthActivity");
	android::SpotifyAuthActivity::javaClass = (jclass)env->NewGlobalRef(spotifyAuthActivityClass);
	android::SpotifyAuthActivity::_performAuthFlow = env->GetStaticMethodID(spotifyAuthActivityClass, "performAuthFlow",
		"(Landroid/content/Context;Lcom/lufinkey/soundholecore/SpotifyLoginOptions;Lcom/lufinkey/soundholecore/SpotifyAuthActivityListener;)V");
	android::SpotifyAuthActivity::_finish = env->GetMethodID(spotifyAuthActivityClass, "finish", "(Lcom/lufinkey/soundholecore/NativeFunction;)V");
	android::SpotifyAuthActivity::_showProgressDialog = env->GetMethodID(spotifyAuthActivityClass, "showProgressDialog", "(Ljava/lang/String;)V");
	android::SpotifyAuthActivity::_hideProgressDialog = env->GetMethodID(spotifyAuthActivityClass, "hideProgressDialog", "()V");

	jclass nativeSpotifyAuthActivityListenerClass = env->FindClass("com/lufinkey/soundholecore/SpotifyNativeAuthActivityListener");
	android::SpotifyNativeAuthActivityListener::javaClass = (jclass)env->NewGlobalRef(nativeSpotifyAuthActivityListenerClass);
	android::SpotifyNativeAuthActivityListener::_instantiate = env->GetMethodID(nativeSpotifyAuthActivityListenerClass, "<init>", "(JJJJ)V");

	jclass spotifyErrorClass = env->FindClass("com/spotify/sdk/android/player/Error");
	android::SpotifyError::javaClass = (jclass)env->NewGlobalRef(spotifyErrorClass);
	android::SpotifyError::_nativeCode = env->GetFieldID(spotifyErrorClass, "nativeCode", "I");
	android::SpotifyError::generateEnumMap(env);

	jclass spotifyUtilsClass = env->FindClass("com/lufinkey/soundholecore/SpotifyUtils");
	android::SpotifyUtils::javaClass = (jclass)env->NewGlobalRef(spotifyUtilsClass);
	android::SpotifyUtils::_instantiate = env->GetMethodID(spotifyUtilsClass, "<init>", "()V");
	android::SpotifyUtils::_getPlayer = env->GetMethodID(spotifyUtilsClass,
		"getPlayer",
		"(Ljava/lang/String;Ljava/lang/String;Lcom/lufinkey/soundholecore/NativeSpotifyPlayerInitCallback;)V");
	android::SpotifyUtils::_destroyPlayer = env->GetMethodID(spotifyUtilsClass,
		"destroyPlayer",
		"(Lcom/spotify/sdk/android/player/SpotifyPlayer;)V");

	jclass spotifyPlayerClass = env->FindClass("com/spotify/sdk/android/player/SpotifyPlayer");
	android::SpotifyPlayer::javaClass = (jclass)env->NewGlobalRef(spotifyPlayerClass);
	android::SpotifyPlayer::_isLoggedIn = env->GetMethodID(spotifyPlayerClass, "isLoggedIn", "()Z");
	android::SpotifyPlayer::_login = env->GetMethodID(spotifyPlayerClass, "login", "(Ljava/lang/String;)Z");
	android::SpotifyPlayer::_logout = env->GetMethodID(spotifyPlayerClass, "logout", "()Z");
	android::SpotifyPlayer::_playUri = env->GetMethodID(spotifyPlayerClass, "playUri", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;Ljava/lang/String;II)V");
	android::SpotifyPlayer::_queue = env->GetMethodID(spotifyPlayerClass, "queue", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;Ljava/lang/String;)V");
	android::SpotifyPlayer::_seekToPosition = env->GetMethodID(spotifyPlayerClass, "seekToPosition", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;I)V");
	android::SpotifyPlayer::_pause = env->GetMethodID(spotifyPlayerClass, "pause", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;)V");
	android::SpotifyPlayer::_resume = env->GetMethodID(spotifyPlayerClass, "resume", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;)V");
	android::SpotifyPlayer::_getPlaybackState = env->GetMethodID(spotifyPlayerClass, "getPlaybackState", "()Lcom/spotify/sdk/android/player/PlaybackState;");
	android::SpotifyPlayer::_getMetadata = env->GetMethodID(spotifyPlayerClass, "getMetadata", "()Lcom/spotify/sdk/android/player/Metadata;");
	android::SpotifyPlayer::_skipToNext = env->GetMethodID(spotifyPlayerClass, "skipToNext", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;)V");
	android::SpotifyPlayer::_skipToPrevious = env->GetMethodID(spotifyPlayerClass, "skipToPrevious", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;)V");
	android::SpotifyPlayer::_setShuffle = env->GetMethodID(spotifyPlayerClass, "setShuffle", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;Z)V");
	android::SpotifyPlayer::_setRepeat = env->GetMethodID(spotifyPlayerClass, "setRepeat", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;Z)V");

	jclass spotifyPlaybackStateClass = env->FindClass("com/spotify/sdk/android/player/PlaybackState");
	android::SpotifyPlaybackState::javaClass = (jclass)env->NewGlobalRef(spotifyPlaybackStateClass);
	android::SpotifyPlaybackState::_isPlaying = env->GetFieldID(spotifyPlaybackStateClass, "isPlaying", "Z");
	android::SpotifyPlaybackState::_isRepeating = env->GetFieldID(spotifyPlaybackStateClass, "isRepeating", "Z");
	android::SpotifyPlaybackState::_isShuffling = env->GetFieldID(spotifyPlaybackStateClass, "isShuffling", "Z");
	android::SpotifyPlaybackState::_isActiveDevice = env->GetFieldID(spotifyPlaybackStateClass, "isActiveDevice", "Z");
	android::SpotifyPlaybackState::_positionMs = env->GetFieldID(spotifyPlaybackStateClass, "positionMs", "J");

	jclass spotifyMetadataClass = env->FindClass("com/spotify/sdk/android/player/Metadata");
	android::SpotifyMetadata::javaClass = (jclass)env->NewGlobalRef(spotifyMetadataClass);
	android::SpotifyMetadata::_contextName = env->GetFieldID(spotifyMetadataClass, "contextName", "Ljava/lang/String;");
	android::SpotifyMetadata::_contextUri = env->GetFieldID(spotifyMetadataClass, "contextUri", "Ljava/lang/String;");
	android::SpotifyMetadata::_prevTrack = env->GetFieldID(spotifyMetadataClass, "prevTrack", "Lcom/spotify/sdk/android/player/Metadata$Track;");
	android::SpotifyMetadata::_currentTrack = env->GetFieldID(spotifyMetadataClass, "currentTrack", "Lcom/spotify/sdk/android/player/Metadata$Track;");
	android::SpotifyMetadata::_nextTrack = env->GetFieldID(spotifyMetadataClass, "nextTrack", "Lcom/spotify/sdk/android/player/Metadata$Track;");

	jclass spotifyTrackClass = env->FindClass("com/spotify/sdk/android/player/Metadata$Track");
	android::SpotifyTrack::javaClass = (jclass)env->NewGlobalRef(spotifyTrackClass);
	android::SpotifyTrack::_name = env->GetFieldID(spotifyTrackClass, "name", "Ljava/lang/String;");
	android::SpotifyTrack::_uri = env->GetFieldID(spotifyTrackClass, "uri", "Ljava/lang/String;");
	android::SpotifyTrack::_artistName = env->GetFieldID(spotifyTrackClass, "artistName", "Ljava/lang/String;");
	android::SpotifyTrack::_artistUri = env->GetFieldID(spotifyTrackClass, "artistUri", "Ljava/lang/String;");
	android::SpotifyTrack::_albumName = env->GetFieldID(spotifyTrackClass, "albumName", "Ljava/lang/String;");
	android::SpotifyTrack::_albumUri = env->GetFieldID(spotifyTrackClass, "albumUri", "Ljava/lang/String;");
	android::SpotifyTrack::_durationMs = env->GetFieldID(spotifyTrackClass, "durationMs", "J");
	android::SpotifyTrack::_indexInContext = env->GetFieldID(spotifyTrackClass, "indexInContext", "J");
	android::SpotifyTrack::_albumCoverWebUrl = env->GetFieldID(spotifyTrackClass, "albumCoverWebUrl", "Ljava/lang/String;");

	// validate that all methods have been got
	for(auto& methodList : {
		android::Utils::methods(), android::NativeFunction::methods(), android::NativeCallback::methods(),
		android::NativeSpotifyPlayerInitCallback::methods(), android::NativeSpotifyPlayerOpCallback::methods(),
		android::SpotifyPlayerEventHandler::methods(), android::SpotifyLoginOptions::methods(),
		android::SpotifyAuthActivity::methods(), android::SpotifyNativeAuthActivityListener::methods(),
		android::SpotifyUtils::methods(), android::SpotifyPlayer::methods()
	}) {
		for(auto method : methodList) {
			if(method == nullptr) {
				throw std::runtime_error("missing java method for Spotify");
			}
		}
	}
	// validate that all fields have been got
	for(auto& fieldList : {
		android::SpotifySession::fields(), android::SpotifyLoginOptions::fields(), android::SpotifyError::fields(),
		android::SpotifyPlaybackState::fields(), android::SpotifyMetadata::fields(), android::SpotifyTrack::fields()
	}) {
		for(auto field : fieldList) {
			if(field == nullptr) {
				throw std::runtime_error("missing java field for Spotify");
			}
		}
	}
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
#endif

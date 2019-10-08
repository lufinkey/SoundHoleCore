//
//  SpotifyPlayer_android.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/4/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#ifdef __ANDROID__

#include <jni.h>
#include "SpotifyPlayer.hpp"
#include <functional>
#include <vector>
#include <soundhole/utils/android/AndroidUtils.hpp>
#include <embed/nodejs/NodeJS_jni.hpp>

#ifdef TARGETPLATFORM_ANDROID

namespace sh {
	namespace android {
		namespace SpotifyUtils {
			jclass javaClass = nullptr;
			jmethodID instantiate = nullptr;
			jmethodID getPlayer = nullptr;
			jmethodID destroyPlayer = nullptr;

			inline std::vector<jmethodID> methods() {
				return {
					instantiate,
					getPlayer, destroyPlayer
				};
			}
		}

		namespace SpotifyPlayer {
			jclass javaClass = nullptr;
			jmethodID isLoggedIn = nullptr;
			jmethodID login = nullptr;
			jmethodID logout = nullptr;
			jmethodID playUri = nullptr;
			jmethodID queue = nullptr;
			jmethodID seekToPosition = nullptr;
			jmethodID pause = nullptr;
			jmethodID resume = nullptr;
			jmethodID getPlaybackState = nullptr;
			jmethodID getMetadata = nullptr;
			jmethodID skipToNext = nullptr;
			jmethodID skipToPrevious = nullptr;
			jmethodID setShuffle = nullptr;
			jmethodID setRepeat = nullptr;

			inline std::vector<jmethodID> methods() {
				return {
						isLoggedIn, login, logout,
						playUri, queue, seekToPosition,
						pause, resume, getPlaybackState, getMetadata,
						skipToNext, skipToPrevious,
						setShuffle, setRepeat
				};
			}
		}

		namespace SpotifyPlaybackState {
			jclass javaClass = nullptr;
			jfieldID isPlaying = nullptr;
			jfieldID isRepeating = nullptr;
			jfieldID isShuffling = nullptr;
			jfieldID isActiveDevice = nullptr;
			jfieldID positionMs = nullptr;

			inline std::vector<jfieldID> fields() {
				return {
					isPlaying, isRepeating, isShuffling, isActiveDevice, positionMs
				};
			}
		}

		namespace SpotifyMetadata {
			jclass javaClass = nullptr;
			jfieldID contextName = nullptr;
			jfieldID contextUri = nullptr;
			jfieldID prevTrack = nullptr;
			jfieldID currentTrack = nullptr;
			jfieldID nextTrack = nullptr;

			inline std::vector<jfieldID> fields() {
				return {
					contextName, contextUri, prevTrack, currentTrack, nextTrack
				};
			}
		}

		namespace SpotifyTrack {
			jclass javaClass = nullptr;
			jfieldID name = nullptr;
			jfieldID uri = nullptr;
			jfieldID artistName = nullptr;
			jfieldID artistUri = nullptr;
			jfieldID albumName = nullptr;
			jfieldID albumUri = nullptr;
			jfieldID durationMs = nullptr;
			jfieldID indexInContext = nullptr;
			jfieldID albumCoverWebUrl = nullptr;

			inline std::vector<jfieldID> fields() {
				return {
					name, uri, artistName, artistUri, albumName, albumUri, durationMs, indexInContext, albumCoverWebUrl
				};
			}
		}
	}

	using ScopedJNIEnv = embed::nodejs::ScopedJNIEnv;

	template<typename... Arg>
	Promise<void> androidPlayerOp(JNIEnv* env, jobject player, jmethodID methodId, Arg... args) {
		if (player == nullptr) {
			return Promise<void>::reject(
				SpotifyError(SpotifyError::Code::SDK_UNINITIALIZED, "Player is not initialized")
			);
		}
		return Promise<void>([=](auto resolve, auto reject) {
			jobject callback = newAndroidSpotifyPlayerOpCallback(env, [=](JNIEnv *env) {
				resolve();
			}, [=](JNIEnv* env, jobject error) {
				reject(SpotifyError(env, error));
			});
			env->CallVoidMethod(player, methodId, callback, args...);
		});
	}




	SpotifyPlayer::SpotifyPlayer()
	: auth(nullptr), spotifyUtils(nullptr), player(nullptr), playerEventHandler(nullptr),
	starting(false), loggingIn(false), loggedIn(false), loggingOut(false), renewingSession(false) {
		auto mainJavaVM = getMainJavaVM();
		if(mainJavaVM == nullptr) {
			throw std::runtime_error("Java VM has not been set");
		} else if(android::SpotifyUtils::javaClass == nullptr) {
			throw std::runtime_error("Java SpotifyUtils class has not been set");
		}
		ScopedJNIEnv scopedEnv(mainJavaVM);
		JNIEnv* env = scopedEnv.getEnv();
		jobject spotifyUtils = env->NewObject(android::SpotifyUtils::javaClass, android::SpotifyUtils::instantiate);
		this->spotifyUtils = env->NewGlobalRef(spotifyUtils);
	}

	SpotifyPlayer::~SpotifyPlayer() {
		setAuth(nullptr);
		logout();
		stop();
		ScopedJNIEnv scopedEnv(getMainJavaVM());
		JNIEnv* env = scopedEnv.getEnv();
		env->DeleteGlobalRef((jobject)spotifyUtils);
		if(player != nullptr) {
			env->DeleteGlobalRef((jobject)player);
		}
	}



	Promise<void> SpotifyPlayer::start() {
		ScopedJNIEnv scopedEnv(getMainJavaVM());
		JNIEnv* env = scopedEnv.getEnv();
		std::unique_lock<std::recursive_mutex> lock(startMutex);
		if(starting) {
			return Promise<void>([=](auto resolve, auto reject) {
				startCallbacks.pushBack({ resolve, reject });
			});
		}
		else if(player != nullptr) {
			return Promise<void>::resolve();
		}
		else if(auth == nullptr) {
			return Promise<void>::reject(
				SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "auth has not been assigned to player")
			);
		}
		auto session = auth->getSession();
		auto resultPromise = Promise<void>([=](auto resolve, auto reject) {
			startCallbacks.pushBack({ resolve, reject });
		});
		starting = true;
		lock.unlock();

		jstring clientId = env->NewStringUTF(auth->getOptions().clientId);
		jstring accessToken = session ? env->NewStringUTF(session->getAccessToken()) : nullptr;
		jobject callback = newAndroidSpotifyPlayerInitCallback(env, [=](JNIEnv* env, jobject player) {
			std::unique_lock<std::recursive_mutex> lock(startMutex);
			bool wasStarting = starting;
			starting = false;
			if(wasStarting) {
				this->player = env->NewGlobalRef(player);
				jobject playerEventHandler = newAndroidSpotifyPlayerEventHandler(env, player, SpotifyPlayerEventHandlerParams{
					.onLoggedIn = [=](JNIEnv* env) {
						this->onStreamingLogin();
					},
					.onLoggedOut = [=](JNIEnv* env) {
						this->onStreamingLogout();
					},
					.onLoginFailed = [=](JNIEnv* env, jobject error) {
						this->onStreamingLoginError(SpotifyError(env, error));
					},
					.onTemporaryError = [=](JNIEnv* env) {
						// TODO handle temporary connection error
					},
					.onConnectionMessage = [=](JNIEnv* env, jstring message) {
						// TODO handle player message
					},
					.onDisconnect = [=](JNIEnv* env) {
						// TODO handle disconnect
					},
					.onReconnect = [=](JNIEnv* env) {
						// TODO handle reconnect
					},
					.onPlaybackEvent = [=](JNIEnv* env, jobject event) {
						// TODO handle playback event
					},
					.onPlaybackError = [=](JNIEnv* env, jobject error) {
						onStreamingError(SpotifyError(env, error));
					}
				});
				this->playerEventHandler = env->NewGlobalRef(playerEventHandler);
			} else {
				env->CallVoidMethod((jobject)spotifyUtils, android::SpotifyUtils::destroyPlayer, player);
			}
			LinkedList<WaitCallback> callbacks;
			callbacks.swap(startCallbacks);
			lock.unlock();

			if(wasStarting) {
				for(auto callback : callbacks) {
					callback.resolve();
				}
			} else {
				auto error = SpotifyError(SpotifyError::Code::SDK_INIT_FAILED, "player start was cancelled");
				for(auto callback : callbacks) {
					callback.reject(error);
				}
			}

		}, [=](JNIEnv* env, jobject javaError) {
			env->CallVoidMethod((jobject)spotifyUtils, android::SpotifyUtils::destroyPlayer, nullptr);

			jclass errorClass = env->GetObjectClass(javaError);
			jmethodID Error_getLocalizedMessage = env->GetMethodID(errorClass, "getLocalizedMessage", "()Ljava/lang/String;");
			auto errorMessage = String(env, (jstring)env->CallObjectMethod(javaError, Error_getLocalizedMessage));
			auto error = SpotifyError(SpotifyError::Code::SDK_INIT_FAILED, errorMessage);

			std::unique_lock<std::recursive_mutex> lock(startMutex);
			starting = false;
			LinkedList<WaitCallback> callbacks;
			callbacks.swap(startCallbacks);
			lock.unlock();

			for(auto callback : callbacks) {
				callback.reject(error);
			}
		});
		env->CallVoidMethod((jobject)spotifyUtils, android::SpotifyUtils::getPlayer, clientId, accessToken, callback);

		return resultPromise;
	}

	void SpotifyPlayer::stop() {
		ScopedJNIEnv scopedEnv(getMainJavaVM());
		JNIEnv* env = scopedEnv.getEnv();
		std::unique_lock<std::recursive_mutex> lock(startMutex);
		starting = false;
		destroyAndroidSpotifyPlayerEventHandler(env, (jobject)playerEventHandler);
		playerEventHandler = nullptr;
		env->CallVoidMethod((jobject)spotifyUtils, android::SpotifyUtils::destroyPlayer, player);
		player = nullptr;
		LinkedList<WaitCallback> callbacks;
		callbacks.swap(startCallbacks);
		lock.unlock();

		auto error = SpotifyError(SpotifyError::Code::SDK_INIT_FAILED, "player start was cancelled");
		for(auto callback : callbacks) {
			callback.reject(error);
		}
	}

	bool SpotifyPlayer::isStarted() const {
		return (player != nullptr);
	}

	bool SpotifyPlayer::isPlayerLoggedIn() const {
		ScopedJNIEnv scopedEnv(getMainJavaVM());
		JNIEnv* env = scopedEnv.getEnv();
		if(player == nullptr) {
			return false;
		}
		return (bool)env->CallBooleanMethod((jobject)player, android::SpotifyPlayer::isLoggedIn);
	}

	void SpotifyPlayer::applyAuthToken(String accessToken) {
		ScopedJNIEnv scopedEnv(getMainJavaVM());
		JNIEnv* env = scopedEnv.getEnv();
		if(player == nullptr) {
			throw std::runtime_error("player has not been started");
		} else {
			jstring oauthToken = env->NewStringUTF(accessToken);
			env->CallBooleanMethod((jobject)player, android::SpotifyPlayer::login, oauthToken);
		}
	}

	Promise<void> SpotifyPlayer::logout() {
		std::unique_lock<std::mutex> lock(loginMutex);
		ScopedJNIEnv scopedEnv(getMainJavaVM());
		if(!loggedIn && !isPlayerLoggedIn()) {
			return Promise<void>::resolve();
		}
		JNIEnv* env = scopedEnv.getEnv();
		return Promise<void>([&](auto resolve, auto reject) {
			loginCallbacks.pushBack({ resolve, reject });
			if(!loggingOut) {
				loggingOut = true;
				lock.unlock();
				env->CallBooleanMethod((jobject)player, android::SpotifyPlayer::logout);
			}
		});
	}



	Promise<void> SpotifyPlayer::playURI(String uri, PlayOptions options) {
		return prepareForCall().then([=]() -> Promise<void> {
			ScopedJNIEnv scopedEnv(getMainJavaVM());
			JNIEnv* env = scopedEnv.getEnv();
			jstring uriStr = env->NewStringUTF(uri);
			jint index = (jint)options.index;
			jint position = (jint)(options.position * 1000.0);
			return androidPlayerOp(env, (jobject)player, android::SpotifyPlayer::playUri, uriStr, index, position);
		});
	}

	Promise<void> SpotifyPlayer::queueURI(String uri) {
		return prepareForCall().then([=]() -> Promise<void> {
			ScopedJNIEnv scopedEnv(getMainJavaVM());
			JNIEnv* env = scopedEnv.getEnv();
			jstring uriStr = env->NewStringUTF(uri);
			return androidPlayerOp(env, (jobject)player, android::SpotifyPlayer::queue, uriStr);
		});
	}

	Promise<void> SpotifyPlayer::skipToNext() {
		return prepareForCall().then([=]() -> Promise<void> {
			ScopedJNIEnv scopedEnv(getMainJavaVM());
			JNIEnv* env = scopedEnv.getEnv();
			return androidPlayerOp(env, (jobject)player, android::SpotifyPlayer::skipToNext);
		});
	}

	Promise<void> SpotifyPlayer::skipToPrevious() {
		return prepareForCall().then([=]() -> Promise<void> {
			ScopedJNIEnv scopedEnv(getMainJavaVM());
			JNIEnv* env = scopedEnv.getEnv();
			return androidPlayerOp(env, (jobject)player, android::SpotifyPlayer::skipToPrevious);
		});
	}

	Promise<void> SpotifyPlayer::seek(double position) {
		return prepareForCall((position != 0.0)).then([=]() -> Promise<void> {
			ScopedJNIEnv scopedEnv(getMainJavaVM());
			JNIEnv* env = scopedEnv.getEnv();
			jint positionInt = (jint)(position * 1000.0);
			return androidPlayerOp(env, (jobject)player, android::SpotifyPlayer::seekToPosition, positionInt);
		});
	}

	Promise<void> SpotifyPlayer::setPlaying(bool playing) {
		return prepareForCall(playing).then([=]() -> Promise<void> {
			ScopedJNIEnv scopedEnv(getMainJavaVM());
			JNIEnv* env = scopedEnv.getEnv();
			if(playing) {
				return androidPlayerOp(env, (jobject)player, android::SpotifyPlayer::resume);
			} else {
				return androidPlayerOp(env, (jobject)player, android::SpotifyPlayer::pause);
			}
		});
	}

	Promise<void> SpotifyPlayer::setShuffling(bool shuffling) {
		return prepareForCall().then([=]() -> Promise<void> {
			ScopedJNIEnv scopedEnv(getMainJavaVM());
			JNIEnv* env = scopedEnv.getEnv();
			jboolean shufflingBool = (jboolean)shuffling;
			return androidPlayerOp(env, (jobject)player, android::SpotifyPlayer::setShuffle, shufflingBool);
		});
	}

	Promise<void> SpotifyPlayer::setRepeating(bool repeating) {
		return prepareForCall().then([=]() -> Promise<void> {
			ScopedJNIEnv scopedEnv(getMainJavaVM());
			JNIEnv* env = scopedEnv.getEnv();
			jboolean repeatingBool = (jboolean)repeating;
			return androidPlayerOp(env, (jobject)player, android::SpotifyPlayer::setRepeat, repeatingBool);
		});
	}

	SpotifyPlayer::State SpotifyPlayer::getState() const {
		if(player == nullptr) {
			return State();
		}
		ScopedJNIEnv scopedEnv(getMainJavaVM());
		JNIEnv* env = scopedEnv.getEnv();
		jobject state = env->CallObjectMethod((jobject)player, android::SpotifyPlayer::getPlaybackState);
		return stateFromAndroidState(env, state);
	}

	SpotifyPlayer::Metadata SpotifyPlayer::getMetadata() const {
		if(player == nullptr) {
			return Metadata();
		}
		ScopedJNIEnv scopedEnv(getMainJavaVM());
		JNIEnv* env = scopedEnv.getEnv();
		jobject metadata = env->CallObjectMethod((jobject)player, android::SpotifyPlayer::getMetadata);
		return metadataFromAndroidMetadata(env, metadata);
	}




	SpotifyPlayer::State SpotifyPlayer::stateFromAndroidState(JNIEnv* env, jobject state) {
		return State{
			.playing = (bool)env->GetBooleanField(state, android::SpotifyPlaybackState::isPlaying),
			.repeating = (bool)env->GetBooleanField(state, android::SpotifyPlaybackState::isRepeating),
			.shuffling = (bool)env->GetBooleanField(state, android::SpotifyPlaybackState::isShuffling),
			.activeDevice = (bool)env->GetBooleanField(state, android::SpotifyPlaybackState::isActiveDevice),
			.position = ((double)env->GetLongField(state, android::SpotifyPlaybackState::positionMs)) / 1000.0
		};
	}

	SpotifyPlayer::Track SpotifyPlayer::trackFromAndroidTrack(JNIEnv* env, jobject track, jobject metadata) {
		jint indexInContext = env->GetIntField(track, android::SpotifyTrack::indexInContext);
		jstring contextName = (jstring)env->GetObjectField(metadata, android::SpotifyMetadata::contextName);
		jstring contextUri = (jstring)env->GetObjectField(metadata, android::SpotifyMetadata::contextUri);
		jstring albumCoverArtURL = (jstring)env->GetObjectField(track, android::SpotifyTrack::albumCoverWebUrl);
		return Track{
			.uri = String(env, (jstring)env->GetObjectField(track, android::SpotifyTrack::uri)),
			.name = String(env, (jstring)env->GetObjectField(track, android::SpotifyTrack::name)),
			.artistURI = String(env, (jstring)env->GetObjectField(track, android::SpotifyTrack::artistUri)),
			.artistName = String(env, (jstring)env->GetObjectField(track, android::SpotifyTrack::artistName)),
			.albumURI = String(env, (jstring)env->GetObjectField(track, android::SpotifyTrack::albumUri)),
            .albumName = String(env, (jstring)env->GetObjectField(track, android::SpotifyTrack::albumName)),
            .albumCoverArtURL = (albumCoverArtURL != nullptr) ? Optional<String>(String(env, albumCoverArtURL)) : std::nullopt,
            .duration = ((double)env->GetLongField(track, android::SpotifyTrack::durationMs)) / 1000.0,
			.indexInContext = (indexInContext >= 0) ? Optional<int>((int)indexInContext) : std::nullopt,
			.contextURI = (indexInContext >= 0 && contextUri != nullptr) ? Optional<String>(String(env, contextUri)) : std::nullopt,
			.contextName = (indexInContext >= 0 && contextName != nullptr) ? Optional<String>(String(env, contextName)) : std::nullopt
		};
	}

	SpotifyPlayer::Metadata SpotifyPlayer::metadataFromAndroidMetadata(JNIEnv* env, jobject metadata) {
		jobject prevTrack = env->GetObjectField(metadata, android::SpotifyMetadata::prevTrack);
		jobject currentTrack = env->GetObjectField(metadata, android::SpotifyMetadata::currentTrack);
		jobject nextTrack = env->GetObjectField(metadata, android::SpotifyMetadata::nextTrack);
		return Metadata{
			.previousTrack = (prevTrack != nullptr) ? Optional<Track>(trackFromAndroidTrack(env, prevTrack, metadata)) : std::nullopt,
			.currentTrack = (currentTrack != nullptr) ? Optional<Track>(trackFromAndroidTrack(env, currentTrack, metadata)) : std::nullopt,
			.nextTrack = (nextTrack != nullptr) ? Optional<Track>(trackFromAndroidTrack(env, nextTrack, metadata)) : std::nullopt
		};
	}
}




extern "C" JNIEXPORT void JNICALL
Java_com_lufinkey_soundholecore_SpotifyUtils_initPlayerUtils(JNIEnv* env, jclass utilsClass, jclass playerClass, jclass stateClass, jclass metadataClass, jclass trackClass) {
	using namespace sh;

	android::SpotifyUtils::javaClass = (jclass)env->NewGlobalRef(utilsClass);
	android::SpotifyUtils::instantiate = env->GetMethodID(utilsClass, "<init>", "()V");
	android::SpotifyUtils::getPlayer = env->GetMethodID(utilsClass,
			"getPlayer",
			"(Ljava/lang/String;Ljava/lang/String;Lcom/lufinkey/soundholecore/NativeSpotifyPlayerInitCallback;)V");
	android::SpotifyUtils::destroyPlayer = env->GetMethodID(utilsClass,
			"destroyPlayer",
			"(Lcom/spotify/sdk/android/player/SpotifyPlayer;)V");

	android::SpotifyPlayer::javaClass = (jclass)env->NewGlobalRef(playerClass);
	android::SpotifyPlayer::isLoggedIn = env->GetMethodID(playerClass, "isLoggedIn", "()Z");
	android::SpotifyPlayer::login = env->GetMethodID(playerClass, "login", "(Ljava/lang/String;)Z");
	android::SpotifyPlayer::logout = env->GetMethodID(playerClass, "logout", "()Z");
	android::SpotifyPlayer::playUri = env->GetMethodID(playerClass, "playUri", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;Ljava/lang/String;II)V");
	android::SpotifyPlayer::queue = env->GetMethodID(playerClass, "queue", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;Ljava/lang/String;)V");
	android::SpotifyPlayer::seekToPosition = env->GetMethodID(playerClass, "seekToPosition", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;I)V");
	android::SpotifyPlayer::pause = env->GetMethodID(playerClass, "pause", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;)V");
	android::SpotifyPlayer::resume = env->GetMethodID(playerClass, "resume", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;)V");
	android::SpotifyPlayer::getPlaybackState = env->GetMethodID(playerClass, "getPlaybackState", "()Lcom/spotify/sdk/android/player/PlaybackState;");
	android::SpotifyPlayer::getMetadata = env->GetMethodID(playerClass, "getMetadata", "()Lcom/spotify/sdk/android/player/Metadata;");
	android::SpotifyPlayer::skipToNext = env->GetMethodID(playerClass, "skipToNext", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;)V");
	android::SpotifyPlayer::skipToNext = env->GetMethodID(playerClass, "skipToPrevious", "(Lcom/spotify/sdk/android/player/Player$OperationCallback;)V");

	android::SpotifyPlaybackState::javaClass = (jclass)env->NewGlobalRef(stateClass);
	android::SpotifyPlaybackState::isPlaying = env->GetFieldID(stateClass, "isPlaying", "Z");
	android::SpotifyPlaybackState::isRepeating = env->GetFieldID(stateClass, "isRepeating", "Z");
	android::SpotifyPlaybackState::isShuffling = env->GetFieldID(stateClass, "isShuffling", "Z");
	android::SpotifyPlaybackState::isActiveDevice = env->GetFieldID(stateClass, "isActiveDevice", "Z");
	android::SpotifyPlaybackState::positionMs = env->GetFieldID(stateClass, "positionMs", "J");

	android::SpotifyMetadata::javaClass = (jclass)env->NewGlobalRef(metadataClass);
	android::SpotifyMetadata::contextName = env->GetFieldID(metadataClass, "contextName", "Ljava/lang/String;");
	android::SpotifyMetadata::contextUri = env->GetFieldID(metadataClass, "contextUri", "Ljava/lang/String;");
	android::SpotifyMetadata::prevTrack = env->GetFieldID(metadataClass, "prevTrack", "Lcom/spotify/sdk/android/player/Metadata$Track;");
	android::SpotifyMetadata::currentTrack = env->GetFieldID(metadataClass, "currentTrack", "Lcom/spotify/sdk/android/player/Metadata$Track;");
	android::SpotifyMetadata::nextTrack = env->GetFieldID(metadataClass, "nextTrack", "Lcom/spotify/sdk/android/player/Metadata$Track;");

	android::SpotifyTrack::javaClass = (jclass)env->NewGlobalRef(trackClass);
	android::SpotifyTrack::name = env->GetFieldID(trackClass, "name", "Ljava/lang/String;");
	android::SpotifyTrack::uri = env->GetFieldID(trackClass, "uri", "Ljava/lang/String;");
	android::SpotifyTrack::artistName = env->GetFieldID(trackClass, "artistName", "Ljava/lang/String;");
	android::SpotifyTrack::artistUri = env->GetFieldID(trackClass, "artistUri", "Ljava/lang/String;");
	android::SpotifyTrack::albumName = env->GetFieldID(trackClass, "albumName", "Ljava/lang/String;");
	android::SpotifyTrack::albumUri = env->GetFieldID(trackClass, "albumUri", "Ljava/lang/String;");
	android::SpotifyTrack::durationMs = env->GetFieldID(trackClass, "durationMs", "J");
	android::SpotifyTrack::indexInContext = env->GetFieldID(trackClass, "indexInContext", "J");
	android::SpotifyTrack::albumCoverWebUrl = env->GetFieldID(trackClass, "albumCoverWebUrl", "Ljava/lang/String;");

	// validate that all methods have been got
	for(auto& methodList : { android::SpotifyUtils::methods(), android::SpotifyPlayer::methods() }) {
		for(auto method : methodList) {
			if(method == nullptr) {
				throw std::runtime_error("missing java method for Spotify");
			}
		}
	}
	// validate that all fields have been got
	for(auto& fieldList : { android::SpotifyPlaybackState::fields(), android::SpotifyMetadata::fields(), android::SpotifyTrack::fields() }) {
		for(auto field : fieldList) {
			if(field == nullptr) {
				throw std::runtime_error("missing java field for Spotify");
			}
		}
	}
}

#endif
#endif

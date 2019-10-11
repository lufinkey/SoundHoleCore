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

#ifdef TARGETPLATFORM_ANDROID

namespace sh {
	template<typename... Arg>
	Promise<void> androidPlayerOp(JNIEnv* env, jobject player, jmethodID methodId, Arg... args) {
		if (player == nullptr) {
			return Promise<void>::reject(
				SpotifyError(SpotifyError::Code::SDK_UNINITIALIZED, "Player is not initialized")
			);
		}
		return Promise<void>([=](auto resolve, auto reject) {
			jobject callback = android::NativeSpotifyPlayerOpCallback::newObject(env, [=](JNIEnv *env) {
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
		jobject spotifyUtils = android::SpotifyUtils::newObject(env);
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
		android::SpotifyUtils::getPlayer(env, (jobject)spotifyUtils,
			clientId, accessToken,
			android::NativeSpotifyPlayerInitCallback::newObject(env, [=](JNIEnv* env, jobject player) {
				std::unique_lock<std::recursive_mutex> lock(startMutex);
				bool wasStarting = starting;
				starting = false;
				if(wasStarting) {
					this->player = env->NewGlobalRef(player);
					jobject playerEventHandler = android::SpotifyPlayerEventHandler::newObject(env, player, {
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
					android::SpotifyUtils::destroyPlayer(env, (jobject)spotifyUtils, player);
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
				android::SpotifyUtils::destroyPlayer(env, (jobject)spotifyUtils, nullptr);

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
			})
		);

		return resultPromise;
	}

	void SpotifyPlayer::stop() {
		ScopedJNIEnv scopedEnv(getMainJavaVM());
		JNIEnv* env = scopedEnv.getEnv();
		std::unique_lock<std::recursive_mutex> lock(startMutex);
		starting = false;
		android::SpotifyPlayerEventHandler::destroy(env, (jobject)playerEventHandler);
		playerEventHandler = nullptr;
		android::SpotifyUtils::destroyPlayer(env, (jobject)spotifyUtils, (jobject)player);
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
		return (bool)android::SpotifyPlayer::isLoggedIn(env, (jobject)player);
	}

	void SpotifyPlayer::applyAuthToken(String accessToken) {
		ScopedJNIEnv scopedEnv(getMainJavaVM());
		JNIEnv* env = scopedEnv.getEnv();
		if(player == nullptr) {
			throw std::runtime_error("player has not been started");
		} else {
			android::SpotifyPlayer::login(env, (jobject)player, accessToken.toJavaString(env));
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
				android::SpotifyPlayer::logout(env, (jobject)player);
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
			return androidPlayerOp(env, (jobject)player, android::SpotifyPlayer::_playUri, uriStr, index, position);
		});
	}

	Promise<void> SpotifyPlayer::queueURI(String uri) {
		return prepareForCall().then([=]() -> Promise<void> {
			ScopedJNIEnv scopedEnv(getMainJavaVM());
			JNIEnv* env = scopedEnv.getEnv();
			jstring uriStr = env->NewStringUTF(uri);
			return androidPlayerOp(env, (jobject)player, android::SpotifyPlayer::_queue, uriStr);
		});
	}

	Promise<void> SpotifyPlayer::skipToNext() {
		return prepareForCall().then([=]() -> Promise<void> {
			ScopedJNIEnv scopedEnv(getMainJavaVM());
			JNIEnv* env = scopedEnv.getEnv();
			return androidPlayerOp(env, (jobject)player, android::SpotifyPlayer::_skipToNext);
		});
	}

	Promise<void> SpotifyPlayer::skipToPrevious() {
		return prepareForCall().then([=]() -> Promise<void> {
			ScopedJNIEnv scopedEnv(getMainJavaVM());
			JNIEnv* env = scopedEnv.getEnv();
			return androidPlayerOp(env, (jobject)player, android::SpotifyPlayer::_skipToPrevious);
		});
	}

	Promise<void> SpotifyPlayer::seek(double position) {
		return prepareForCall((position != 0.0)).then([=]() -> Promise<void> {
			ScopedJNIEnv scopedEnv(getMainJavaVM());
			JNIEnv* env = scopedEnv.getEnv();
			jint positionInt = (jint)(position * 1000.0);
			return androidPlayerOp(env, (jobject)player, android::SpotifyPlayer::_seekToPosition, positionInt);
		});
	}

	Promise<void> SpotifyPlayer::setPlaying(bool playing) {
		return prepareForCall(playing).then([=]() -> Promise<void> {
			ScopedJNIEnv scopedEnv(getMainJavaVM());
			JNIEnv* env = scopedEnv.getEnv();
			if(playing) {
				return androidPlayerOp(env, (jobject)player, android::SpotifyPlayer::_resume);
			} else {
				return androidPlayerOp(env, (jobject)player, android::SpotifyPlayer::_pause);
			}
		});
	}

	Promise<void> SpotifyPlayer::setShuffling(bool shuffling) {
		return prepareForCall().then([=]() -> Promise<void> {
			ScopedJNIEnv scopedEnv(getMainJavaVM());
			JNIEnv* env = scopedEnv.getEnv();
			jboolean shufflingBool = (jboolean)shuffling;
			return androidPlayerOp(env, (jobject)player, android::SpotifyPlayer::_setShuffle, shufflingBool);
		});
	}

	Promise<void> SpotifyPlayer::setRepeating(bool repeating) {
		return prepareForCall().then([=]() -> Promise<void> {
			ScopedJNIEnv scopedEnv(getMainJavaVM());
			JNIEnv* env = scopedEnv.getEnv();
			jboolean repeatingBool = (jboolean)repeating;
			return androidPlayerOp(env, (jobject)player, android::SpotifyPlayer::_setRepeat, repeatingBool);
		});
	}

	SpotifyPlayer::State SpotifyPlayer::getState() const {
		if(player == nullptr) {
			return State();
		}
		ScopedJNIEnv scopedEnv(getMainJavaVM());
		JNIEnv* env = scopedEnv.getEnv();
		jobject state = env->CallObjectMethod((jobject)player, android::SpotifyPlayer::_getPlaybackState);
		return stateFromAndroidState(env, state);
	}

	SpotifyPlayer::Metadata SpotifyPlayer::getMetadata() const {
		if(player == nullptr) {
			return Metadata();
		}
		ScopedJNIEnv scopedEnv(getMainJavaVM());
		JNIEnv* env = scopedEnv.getEnv();
		jobject metadata = env->CallObjectMethod((jobject)player, android::SpotifyPlayer::_getMetadata);
		return metadataFromAndroidMetadata(env, metadata);
	}




	SpotifyPlayer::State SpotifyPlayer::stateFromAndroidState(JNIEnv* env, jobject state) {
		return State{
			.playing = (bool)env->GetBooleanField(state, android::SpotifyPlaybackState::_isPlaying),
			.repeating = (bool)env->GetBooleanField(state, android::SpotifyPlaybackState::_isRepeating),
			.shuffling = (bool)env->GetBooleanField(state, android::SpotifyPlaybackState::_isShuffling),
			.activeDevice = (bool)env->GetBooleanField(state, android::SpotifyPlaybackState::_isActiveDevice),
			.position = ((double)env->GetLongField(state, android::SpotifyPlaybackState::_positionMs)) / 1000.0
		};
	}

	SpotifyPlayer::Track SpotifyPlayer::trackFromAndroidTrack(JNIEnv* env, jobject track, jobject metadata) {
		jint indexInContext = env->GetIntField(track, android::SpotifyTrack::_indexInContext);
		jstring contextName = (jstring)env->GetObjectField(metadata, android::SpotifyMetadata::_contextName);
		jstring contextUri = (jstring)env->GetObjectField(metadata, android::SpotifyMetadata::_contextUri);
		jstring albumCoverArtURL = (jstring)env->GetObjectField(track, android::SpotifyTrack::_albumCoverWebUrl);
		return Track{
			.uri = String(env, (jstring)env->GetObjectField(track, android::SpotifyTrack::_uri)),
			.name = String(env, (jstring)env->GetObjectField(track, android::SpotifyTrack::_name)),
			.artistURI = String(env, (jstring)env->GetObjectField(track, android::SpotifyTrack::_artistUri)),
			.artistName = String(env, (jstring)env->GetObjectField(track, android::SpotifyTrack::_artistName)),
			.albumURI = String(env, (jstring)env->GetObjectField(track, android::SpotifyTrack::_albumUri)),
            .albumName = String(env, (jstring)env->GetObjectField(track, android::SpotifyTrack::_albumName)),
            .albumCoverArtURL = (albumCoverArtURL != nullptr) ? Optional<String>(String(env, albumCoverArtURL)) : std::nullopt,
            .duration = ((double)env->GetLongField(track, android::SpotifyTrack::_durationMs)) / 1000.0,
			.indexInContext = (indexInContext >= 0) ? Optional<int>((int)indexInContext) : std::nullopt,
			.contextURI = (indexInContext >= 0 && contextUri != nullptr) ? Optional<String>(String(env, contextUri)) : std::nullopt,
			.contextName = (indexInContext >= 0 && contextName != nullptr) ? Optional<String>(String(env, contextName)) : std::nullopt
		};
	}

	SpotifyPlayer::Metadata SpotifyPlayer::metadataFromAndroidMetadata(JNIEnv* env, jobject metadata) {
		jobject prevTrack = env->GetObjectField(metadata, android::SpotifyMetadata::_prevTrack);
		jobject currentTrack = env->GetObjectField(metadata, android::SpotifyMetadata::_currentTrack);
		jobject nextTrack = env->GetObjectField(metadata, android::SpotifyMetadata::_nextTrack);
		return Metadata{
			.previousTrack = (prevTrack != nullptr) ? Optional<Track>(trackFromAndroidTrack(env, prevTrack, metadata)) : std::nullopt,
			.currentTrack = (currentTrack != nullptr) ? Optional<Track>(trackFromAndroidTrack(env, currentTrack, metadata)) : std::nullopt,
			.nextTrack = (nextTrack != nullptr) ? Optional<Track>(trackFromAndroidTrack(env, nextTrack, metadata)) : std::nullopt
		};
	}
}

#endif
#endif

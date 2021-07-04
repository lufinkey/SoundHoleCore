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
#include <soundhole/jnicpp/android/spotify/NativeSpotifyPlayerInitCallback_jni.hpp>
#include <soundhole/jnicpp/android/spotify/NativeSpotifyPlayerOpCallback_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyPlayer_jni.hpp>
#include <functional>
#include <vector>


namespace sh {
	namespace jni {
		using namespace android::spotify;
	}

	template<typename... Arg>
	Promise<void> androidPlayerOp(JNIEnv* env, jobject player, jmethodID methodId, Arg... args) {
		if (player == nullptr) {
			return Promise<void>::reject(
				SpotifyError(SpotifyError::Code::SDK_UNINITIALIZED, "Player is not initialized")
			);
		}
		return Promise<void>([=](auto resolve, auto reject) {
			jobject callback = jni::NativeSpotifyPlayerOpCallback::newObject(env, [=](JNIEnv *env) {
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
		auto vm = getJavaVM();
		if(vm == nullptr) {
			throw std::runtime_error("Java VM has not been set");
		}
		jniScope(vm, [=](JNIEnv* env) {
			auto spotifyUtils = jni::SpotifyUtils::newObject(env);
			this->spotifyUtils.value = env->NewGlobalRef(spotifyUtils);
		});
	}

	SpotifyPlayer::~SpotifyPlayer() {
		jniScope(getJavaVM(), [&](JNIEnv* env) {
			setAuth(nullptr);
			logout();
			stop();
			env->DeleteGlobalRef(spotifyUtils.value);
			if(player.value != nullptr) {
				env->DeleteGlobalRef(player.value);
			}
		});
	}



	Promise<void> SpotifyPlayer::start() {
		return jniScope(getJavaVM(), [=](JNIEnv* env) {
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

			jstring clientId = auth->getOptions().clientId.toJavaString(env);
			jstring accessToken = session ? session->getAccessToken().toJavaString(env) : nullptr;
			spotifyUtils.getPlayer(env, clientId, accessToken,
				jni::NativeSpotifyPlayerInitCallback::newObject(env, [=](JNIEnv* env, jni::SpotifyPlayer player) {
					std::unique_lock<std::recursive_mutex> lock(startMutex);
					bool wasStarting = starting;
					starting = false;
					if(wasStarting) {
						this->player.value = env->NewGlobalRef(player);
						jobject playerEventHandler = jni::SpotifyPlayerEventHandler::newObject(env, {
							.player = player,
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
						this->playerEventHandler.value = env->NewGlobalRef(playerEventHandler);
					} else {
						spotifyUtils.destroyPlayer(env, player);
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
					spotifyUtils.destroyPlayer(env, nullptr);

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
		});
	}

	void SpotifyPlayer::stop() {
		return jniScope(getJavaVM(), [=](JNIEnv* env) {
			std::unique_lock<std::recursive_mutex> lock(startMutex);
			starting = false;
			playerEventHandler.destroy(env);
			playerEventHandler.value = nullptr;
			spotifyUtils.destroyPlayer(env, player);
			player.value = nullptr;
			LinkedList<WaitCallback> callbacks;
			callbacks.swap(startCallbacks);
			lock.unlock();

			auto error = SpotifyError(SpotifyError::Code::SDK_INIT_FAILED, "player start was cancelled");
			for(auto callback : callbacks) {
				callback.reject(error);
			}
		});
	}

	bool SpotifyPlayer::isStarted() const {
		return (player != nullptr);
	}

	bool SpotifyPlayer::isPlayerLoggedIn() const {
		return jniScope(getJavaVM(), [=](JNIEnv* env) {
			auto player = jni::SpotifyPlayer(this->player.value);
			if(player.value == nullptr) {
				return false;
			}
			return (bool)player.isLoggedIn(env);
		});
	}

	void SpotifyPlayer::applyAuthToken(String accessToken) {
		return jniScope(getJavaVM(), [=](JNIEnv* env) {
			if(player.value == nullptr) {
				throw std::runtime_error("player has not been started");
			} else {
				player.login(env, accessToken.toJavaString(env));
			}
		});
	}

	Promise<void> SpotifyPlayer::logout() {
		return jniScope(getJavaVM(), [=](JNIEnv* env) {
			std::unique_lock<std::mutex> lock(loginMutex);
			if(!loggedIn && !isPlayerLoggedIn()) {
				return Promise<void>::resolve();
			}
			return Promise<void>([&](auto resolve, auto reject) {
				loginCallbacks.pushBack({ resolve, reject });
				if(!loggingOut) {
					loggingOut = true;
					lock.unlock();
					player.logout(env);
				}
			});
		});
	}



	Promise<void> SpotifyPlayer::playURI(String uri, PlayOptions options) {
		return prepareForCall().then([=]() -> Promise<void> {
			return jniScope(getJavaVM(), [=](JNIEnv* env) {
				jstring uriStr = uri.toJavaString(env);
				jint index = (jint)options.index;
				jint position = (jint)(options.position * 1000.0);
				return androidPlayerOp(env, player, jni::SpotifyPlayer::methodID_playUri(env), uriStr, index, position);
			});
		});
	}

	Promise<void> SpotifyPlayer::queueURI(String uri) {
		return prepareForCall().then([=]() -> Promise<void> {
			return jniScope(getJavaVM(), [=](JNIEnv* env) {
				jstring uriStr = uri.toJavaString(env);
				return androidPlayerOp(env, player, jni::SpotifyPlayer::methodID_queue(env), uriStr);
			});
		});
	}

	Promise<void> SpotifyPlayer::skipToNext() {
		return prepareForCall().then([=]() -> Promise<void> {
			return jniScope(getJavaVM(), [=](JNIEnv* env) {
				return androidPlayerOp(env, player, jni::SpotifyPlayer::methodID_skipToNext(env));
			});
		});
	}

	Promise<void> SpotifyPlayer::skipToPrevious() {
		return prepareForCall().then([=]() -> Promise<void> {
			return jniScope(getJavaVM(), [=](JNIEnv* env) {
				return androidPlayerOp(env, player, jni::SpotifyPlayer::methodID_skipToPrevious(env));
			});
		});
	}

	Promise<void> SpotifyPlayer::seek(double position) {
		return prepareForCall((position != 0.0)).then([=]() -> Promise<void> {
			return jniScope(getJavaVM(), [=](JNIEnv* env) {
				jint positionInt = (jint)(position * 1000.0);
				return androidPlayerOp(env, player, jni::SpotifyPlayer::methodID_seekToPosition(env), positionInt);
			});
		});
	}

	Promise<void> SpotifyPlayer::setPlaying(bool playing) {
		return prepareForCall(playing).then([=]() -> Promise<void> {
			return jniScope(getJavaVM(), [=](JNIEnv* env) {
				if (playing) {
					return androidPlayerOp(env, player, jni::SpotifyPlayer::methodID_resume(env));
				} else {
					return androidPlayerOp(env, player, jni::SpotifyPlayer::methodID_pause(env));
				}
			});
		});
	}

	Promise<void> SpotifyPlayer::setShuffling(bool shuffling) {
		return prepareForCall().then([=]() -> Promise<void> {
			return jniScope(getJavaVM(), [=](JNIEnv* env) {
				jboolean shufflingBool = (jboolean)shuffling;
				return androidPlayerOp(env, player, jni::SpotifyPlayer::methodID_setShuffle(env), shufflingBool);
			});
		});
	}

	Promise<void> SpotifyPlayer::setRepeating(bool repeating) {
		return prepareForCall().then([=]() -> Promise<void> {
			return jniScope(getJavaVM(), [=](JNIEnv* env) {
				jboolean repeatingBool = (jboolean)repeating;
				return androidPlayerOp(env, (jobject)player, jni::SpotifyPlayer::methodID_setRepeat(env), repeatingBool);
			});
		});
	}

	SpotifyPlayer::State SpotifyPlayer::getState() const {
		if(player.value == nullptr) {
			return State();
		}
		return jniScope(getJavaVM(), [=](JNIEnv* env) {
			jobject state = env->CallObjectMethod((jobject)player, jni::SpotifyPlayer::methodID_getPlaybackState(env));
			return stateFromAndroidState(env, jni::SpotifyPlaybackState(state));
		});
	}

	SpotifyPlayer::Metadata SpotifyPlayer::getMetadata() const {
		if(player == nullptr) {
			return Metadata();
		}
		return jniScope(getJavaVM(), [=](JNIEnv* env) {
			jobject metadata = env->CallObjectMethod((jobject)player, jni::SpotifyPlayer::methodID_getMetadata(env));
			return metadataFromAndroidMetadata(env, jni::SpotifyMetadata(metadata));
		});
	}




	SpotifyPlayer::State SpotifyPlayer::stateFromAndroidState(JNIEnv* env, jni::SpotifyPlaybackState state) {
		return State{
			.playing = (bool)state.isPlaying(env),
			.repeating = (bool)state.isRepeating(env),
			.shuffling = (bool)state.isShuffling(env),
			.activeDevice = (bool)state.isActiveDevice(env),
			.position = ((double)state.getPositionMs(env)) / 1000.0
		};
	}

	SpotifyPlayer::Track SpotifyPlayer::trackFromAndroidTrack(JNIEnv* env, jni::SpotifyTrack track, jni::SpotifyMetadata metadata) {
		jlong indexInContext = track.getIndexInContext(env);
		jstring contextName = metadata.getContextName(env);
		jstring contextUri = metadata.getContextURI(env);
		jstring albumCoverArtURL = track.getAlbumCoverWebURL(env);
		return Track{
			.uri = String(env, track.getURI(env)),
			.name = String(env, track.getName(env)),
			.artistURI = String(env, track.getArtistURI(env)),
			.artistName = String(env, track.getArtistName(env)),
			.albumURI = String(env, track.getAlbumURI(env)),
            .albumName = String(env, track.getAlbumName(env)),
            .albumCoverArtURL = (albumCoverArtURL != nullptr) ? maybe(String(env, albumCoverArtURL)) : std::nullopt,
            .duration = ((double)track.getDurationMs(env)) / 1000.0,
			.indexInContext = (indexInContext >= 0) ? Optional<int>((int)indexInContext) : std::nullopt,
			.contextURI = (indexInContext >= 0 && contextUri != nullptr) ? Optional<String>(String(env, contextUri)) : std::nullopt,
			.contextName = (indexInContext >= 0 && contextName != nullptr) ? Optional<String>(String(env, contextName)) : std::nullopt
		};
	}

	SpotifyPlayer::Metadata SpotifyPlayer::metadataFromAndroidMetadata(JNIEnv* env, jni::SpotifyMetadata metadata) {
		auto prevTrack = metadata.getPreviousTrack(env);
		auto currentTrack = metadata.getCurrentTrack(env);
		auto nextTrack = metadata.getNextTrack(env);
		return Metadata{
			.previousTrack = (prevTrack.value != nullptr) ? Optional<Track>(trackFromAndroidTrack(env, prevTrack, metadata)) : std::nullopt,
			.currentTrack = (currentTrack.value != nullptr) ? Optional<Track>(trackFromAndroidTrack(env, currentTrack, metadata)) : std::nullopt,
			.nextTrack = (nextTrack.value != nullptr) ? Optional<Track>(trackFromAndroidTrack(env, nextTrack, metadata)) : std::nullopt
		};
	}
}

#endif

//
//  StreamPlayer_android.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/19/20.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#ifdef __ANDROID__

#include <jni.h>
#include "StreamPlayer.hpp"
#include <soundhole/jnicpp/android/AudioManager_jni.hpp>
#include <android/log.h>

namespace sh {
	namespace jni {
		using namespace android;
	}

	StreamPlayer::StreamPlayer()
	: javaVm(getJavaVM()), player(nullptr), preparedPlayer(nullptr) {
		//
	}

	StreamPlayer::~StreamPlayer() {
		playQueue.cancelAllTasks();
		jniScope((JavaVM*)javaVm, [&](JNIEnv* env) {
			destroyPlayer(env);
			destroyPreparedPlayer(env);
		});
	}



	jni::android::MediaPlayer StreamPlayer::createPlayer(JNIEnv* env, String audioURL) {
		auto player = jni::MediaPlayer::newObject(env);
		player.setAudioStreamType(env, jni::android::AudioManager::STREAM_MUSIC(env));
		player.setDataSource(env, audioURL.toJavaString(env));
		player.value = env->NewGlobalRef(player.value);
		return player;
	}

	void StreamPlayer::setPlayer(JNIEnv* env, jni::android::MediaPlayer player, String audioURL) {
		std::unique_lock<std::recursive_mutex> lock(playerMutex);
		destroyPlayer(env);
		this->player = player;
		this->playerAudioURL = audioURL;
		// TODO subscribe to events
	}

	void StreamPlayer::destroyPlayer(JNIEnv* env) {
		std::unique_lock<std::recursive_mutex> lock(playerMutex);
		if(player == nullptr) {
			return;
		}
		auto deadPlayer = player;
		player.value = nullptr;
		playerAudioURL.clear();
		// TODO unsubscribe from events
		lock.unlock();
		deadPlayer.stop(env);
		deadPlayer.reset(env);
		env->DeleteGlobalRef(deadPlayer.value);
	}

	void StreamPlayer::destroyPreparedPlayer(JNIEnv* env) {
		std::unique_lock<std::recursive_mutex> lock(playerMutex);
		if(preparedPlayer == nullptr) {
			return;
		}
		auto deadPlayer = preparedPlayer;
		preparedPlayer.value = nullptr;
		preparedAudioURL.clear();
		lock.unlock();
		env->DeleteGlobalRef(deadPlayer.value);
	}



	Promise<void> StreamPlayer::prepare(String audioURL) {
		if(audioURL.empty()) {
			return Promise<void>::reject(std::invalid_argument("audioURL cannot be empty"));
		}
		std::unique_lock<std::recursive_mutex> lock(playerMutex);
		if(preparedAudioURL == audioURL || playerAudioURL == audioURL) {
			return Promise<void>::resolve();
		}
		lock.unlock();
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "prepare",
			.cancelMatchingTags = true
		};
		return playQueue.run(runOptions, [=](auto task) -> Promise<void> {
			return jniScope(getJavaVM(), [=](JNIEnv* env) {
				std::unique_lock<std::recursive_mutex> lock(playerMutex);
				if(preparedAudioURL == audioURL || playerAudioURL == audioURL) {
					return Promise<void>::resolve();
				}
				destroyPreparedPlayer(env);
				preparedPlayer = createPlayer(env, audioURL);
				preparedAudioURL = audioURL;
				lock.unlock();
				return promiseThread([=]() {
					jniScope((JavaVM*)javaVm, [=](JNIEnv* env) {
						preparedPlayer.prepare(env);
					});
				});
			});
		}).promise;
	}

	Promise<void> StreamPlayer::play(String audioURL, PlayOptions options) {
		if(audioURL.empty()) {
			return Promise<void>::reject(std::invalid_argument("audioURL cannot be empty"));
		}
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "play",
			.cancelMatchingTags = true
		};
		return playQueue.run(runOptions, [=](auto task) -> Promise<void> {
			return jniScope(getJavaVM(), [=](JNIEnv* env) {
				bool needsPrepare = false;
				if(playerAudioURL == audioURL) {
					FGL_ASSERT(player != nullptr, "player is null but playerAudioURL is not empty");
					jint currentTime = player.getCurrentPosition(env);
					if(currentTime != 0) {
						player.seekTo(env, 0);
						jboolean playing = player.isPlaying(env);
						if(!playing) {
							player.start(env);
						}
					}
					return Promise<void>::resolve();
				} else if(preparedAudioURL == audioURL) {
					std::unique_lock<std::recursive_mutex> lock(playerMutex);
					auto newPlayer = preparedPlayer;
					preparedPlayer.value = nullptr;
					preparedAudioURL.clear();
					setPlayer(env, newPlayer, audioURL);
					lock.unlock();
				} else {
					std::unique_lock<std::recursive_mutex> lock(playerMutex);
					destroyPreparedPlayer(env);
					setPlayer(env, createPlayer(env, audioURL), audioURL);
					lock.unlock();
					needsPrepare = true;
				}
				if(options.beforePlay) {
					options.beforePlay();
				}
				FGL_ASSERT(player != nullptr, "player is null after setting up for playback");
				jint currentTime = player.getCurrentPosition(env);
				double playerCurrentPos = (double)currentTime / 1000.0;
				if(needsPrepare) {
					return promiseThread([=]() {
						jniScope((JavaVM*)javaVm, [&](JNIEnv* env) {
							__android_log_print(ANDROID_LOG_DEBUG, "StreamPlayer", "preparing media player with url %s", audioURL.c_str());
							player.prepare(env);
							__android_log_print(ANDROID_LOG_DEBUG, "StreamPlayer", "starting media player");
							player.start(env);
							__android_log_print(ANDROID_LOG_DEBUG, "StreamPlayer", "done starting media player");
						});
					});
				}
				if(playerCurrentPos != options.position) {
					player.seekTo(env, (jint)(options.position * 1000));
				}
				player.start(env);
				return Promise<void>::resolve();
			});
		}).promise;
	}

	Promise<void> StreamPlayer::setPlaying(bool playing) {
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "setPlaying",
			.cancelMatchingTags = true
		};
		return playQueue.run(runOptions, [=](auto task) {
			if(player.value == nullptr) {
				return;
			}
			jniScope(getJavaVM(), [=](JNIEnv* env) {
				if (playing) {
					player.start(env);
				} else {
					player.pause(env);
				}
			});
		}).promise;
	}

	Promise<void> StreamPlayer::seek(double position) {
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "seek",
			.cancelMatchingTags = true
		};
		return playQueue.run(runOptions, [=](auto task) {
			if(player.value == nullptr) {
				return;
			}
			jniScope(getJavaVM(), [=](JNIEnv* env) {
				player.seekTo(env, (jint)(position * 1000));
			});
		}).promise;
	}

	Promise<void> StreamPlayer::stop() {
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "stop",
			.cancelMatchingTags = true
		};
		playQueue.cancelAllTasks();
		return playQueue.run(runOptions, [=](auto task) {
			std::unique_lock<std::recursive_mutex> lock(playerMutex);
			jniScope(getJavaVM(), [=](JNIEnv* env) {
				destroyPlayer(env);
				destroyPreparedPlayer(env);
			});
		}).promise;
	}

	StreamPlayer::PlaybackState StreamPlayer::getState() const {
		std::unique_lock<std::recursive_mutex> lock(playerMutex);
		if(this->player.value == nullptr) {
			return {
				.playing = false,
				.position = 0.0,
				.duration = 0.0
			};
		}
		jint currentTime = 0;
		jboolean playing = false;
		jint currentDuration = 0;
		auto player = jni::MediaPlayer(this->player.value);
		jniScope(getJavaVM(), [&](JNIEnv* env) {
			currentTime = player.getCurrentPosition(env);
			playing = player.isPlaying(env);
			currentDuration = player.getDuration(env);
		});
		return {
			.playing = (bool)playing,
			.position = (double)currentTime / 1000.0,
			.duration = (double)currentDuration / 1000.0
		};
	}
}

#endif

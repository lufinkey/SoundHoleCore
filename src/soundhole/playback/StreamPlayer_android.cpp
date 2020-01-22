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
#include <soundhole/utils/android/AndroidUtils.hpp>
#include <android/log.h>

#ifdef TARGETPLATFORM_ANDROID

namespace sh {
	StreamPlayer::StreamPlayer()
	: javaVm(getMainJavaVM()), player(nullptr), preparedPlayer(nullptr) {
		//
	}

	StreamPlayer::~StreamPlayer() {
		playQueue.cancelAllTasks();
		DispatchQueue::jniScope((JavaVM*)javaVm, [&](JNIEnv* env) {
			destroyPlayer(env);
			destroyPreparedPlayer(env);
		});
	}



	jobject StreamPlayer::createPlayer(JNIEnv* env, String audioURL) {
		jobject player = android::MediaPlayer::newObject(env);
		env->CallVoidMethod(player, android::MediaPlayer::_setAudioStreamType, (jint)android::AudioManager::STREAM_MUSIC(env));
		env->CallVoidMethod(player, android::MediaPlayer::_setDataSource, audioURL.toJavaString(env));
		return env->NewGlobalRef(player);
	}

	void StreamPlayer::setPlayer(JNIEnv* env, jobject player, String audioURL) {
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
		jobject deadPlayer = (jobject)player;
		player = nullptr;
		playerAudioURL.clear();
		// TODO unsubscribe from events
		lock.unlock();
		env->CallVoidMethod(deadPlayer, android::MediaPlayer::_stop);
		env->CallVoidMethod(deadPlayer, android::MediaPlayer::_reset);
		env->DeleteGlobalRef(deadPlayer);
	}

	void StreamPlayer::destroyPreparedPlayer(JNIEnv* env) {
		std::unique_lock<std::recursive_mutex> lock(playerMutex);
		if(preparedPlayer == nullptr) {
			return;
		}
		jobject deadPlayer = (jobject)preparedPlayer;
		preparedPlayer = nullptr;
		preparedAudioURL.clear();
		lock.unlock();
		env->DeleteGlobalRef(deadPlayer);
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
			ScopedJNIEnv scopedEnv(getMainJavaVM());
			JNIEnv* env = scopedEnv.getEnv();
			std::unique_lock<std::recursive_mutex> lock(playerMutex);
			if(preparedAudioURL == audioURL || playerAudioURL == audioURL) {
				return Promise<void>::resolve();
			}
			destroyPreparedPlayer(env);
			preparedPlayer = createPlayer(env, audioURL);
			preparedAudioURL = audioURL;
			lock.unlock();
			return async<void>([=]() {
				DispatchQueue::jniScope((JavaVM*)javaVm, [&](JNIEnv* env) {
					env->CallVoidMethod((jobject)preparedPlayer, android::MediaPlayer::_prepare);
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
			ScopedJNIEnv scopedEnv(getMainJavaVM());
			JNIEnv* env = scopedEnv.getEnv();
			bool needsPrepare = false;
			if(playerAudioURL == audioURL) {
				FGL_ASSERT(player != nullptr, "player is null but playerAudioURL is not empty");
				jint currentTime = env->CallIntMethod((jobject)player, android::MediaPlayer::_getCurrentPosition);
				if(currentTime != 0) {
					env->CallVoidMethod((jobject)player, android::MediaPlayer::_seekTo, (jint)0);
					jboolean playing = env->CallBooleanMethod((jobject)player, android::MediaPlayer::_isPlaying);
					if(!playing) {
						env->CallVoidMethod((jobject)player, android::MediaPlayer::_start);
					}
				}
				return Promise<void>::resolve();
			} else if(preparedAudioURL == audioURL) {
				std::unique_lock<std::recursive_mutex> lock(playerMutex);
				jobject newPlayer = (jobject)preparedPlayer;
				preparedPlayer = nullptr;
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
			jint currentTime = env->CallIntMethod((jobject)player, android::MediaPlayer::_getCurrentPosition);
			double playerCurrentPos = (double)currentTime / 1000.0;
			if(needsPrepare) {
				return async<void>([=]() {
					DispatchQueue::jniScope((JavaVM*)javaVm, [&](JNIEnv* env) {
						__android_log_print(ANDROID_LOG_DEBUG, "StreamPlayer", "preparing media player with url %s", audioURL.c_str());
						env->CallVoidMethod((jobject)player, android::MediaPlayer::_prepare);
						__android_log_print(ANDROID_LOG_DEBUG, "StreamPlayer", "starting media player");
						env->CallVoidMethod((jobject)player, android::MediaPlayer::_start);
						__android_log_print(ANDROID_LOG_DEBUG, "StreamPlayer", "done starting media player");
					});
				});
			}
			if(playerCurrentPos != options.position) {
				env->CallVoidMethod((jobject)player, android::MediaPlayer::_seekTo, (jint)(options.position * 1000));
			}
			env->CallVoidMethod((jobject)player, android::MediaPlayer::_start);
			return Promise<void>::resolve();
		}).promise;
	}

	Promise<void> StreamPlayer::setPlaying(bool playing) {
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "setPlaying",
			.cancelMatchingTags = true
		};
		return playQueue.run(runOptions, [=](auto task) {
			if(player == nullptr) {
				return;
			}
			ScopedJNIEnv scopedEnv(getMainJavaVM());
			JNIEnv* env = scopedEnv.getEnv();
			if(playing) {
				env->CallVoidMethod((jobject)player, android::MediaPlayer::_start);
			} else {
				env->CallVoidMethod((jobject)player, android::MediaPlayer::_pause);
			}
		}).promise;
	}

	Promise<void> StreamPlayer::seek(double position) {
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "seek",
			.cancelMatchingTags = true
		};
		return playQueue.run(runOptions, [=](auto task) {
			if(player == nullptr) {
				return;
			}
			ScopedJNIEnv scopedEnv(getMainJavaVM());
			JNIEnv* env = scopedEnv.getEnv();
			env->CallVoidMethod((jobject)player, android::MediaPlayer::_seekTo, (jint)(position * 1000));
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
			ScopedJNIEnv scopedEnv(getMainJavaVM());
			JNIEnv* env = scopedEnv.getEnv();
			destroyPlayer(env);
			destroyPreparedPlayer(env);
		}).promise;
	}

	StreamPlayer::PlaybackState StreamPlayer::getState() const {
		std::unique_lock<std::recursive_mutex> lock(playerMutex);
		if(player == nullptr) {
			return {
				.playing = false,
				.position = 0.0,
				.duration = 0.0
			};
		}
		ScopedJNIEnv scopedEnv(getMainJavaVM());
		JNIEnv* env = scopedEnv.getEnv();
		jint currentTime = env->CallIntMethod((jobject)player, android::MediaPlayer::_getCurrentPosition);
		jboolean playing = env->CallBooleanMethod((jobject)player, android::MediaPlayer::_isPlaying);
		jint currentDuration = env->CallIntMethod((jobject)player, android::MediaPlayer::_getDuration);
		return {
			.playing = (bool)playing,
			.position = (double)currentTime / 1000.0,
			.duration = (double)currentDuration / 1000.0
		};
	}
}

#endif
#endif

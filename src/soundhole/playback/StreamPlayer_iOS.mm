//
//  StreamPlayer_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 12/13/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "StreamPlayer.hpp"

#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

namespace sh {
	StreamPlayer::StreamPlayer()
	: player(nil), preparedPlayer(nil), playerEventHandler([[StreamPlayerEventHandler alloc] init]) {
		playerEventHandler.onPlay = ^{
			std::unique_lock<std::mutex> lock(listenersMutex);
			auto listeners = this->listeners;
			lock.unlock();
			for(auto listener : listeners) {
				listener->onStreamPlayerPlay(this);
			}
		};
		playerEventHandler.onPause = ^{
			std::unique_lock<std::mutex> lock(listenersMutex);
			auto listeners = this->listeners;
			lock.unlock();
			for(auto listener : listeners) {
				listener->onStreamPlayerPause(this);
			}
		};
		playerEventHandler.onItemFinish = ^(AVPlayerItem* _Nonnull item) {
			std::unique_lock<std::recursive_mutex> playerLock(playerMutex);
			auto audioURL = playerAudioURL;
			if(item != player.currentItem) {
				return;
			}
			playerLock.unlock();
			
			std::unique_lock<std::mutex> lock(listenersMutex);
			auto listeners = this->listeners;
			lock.unlock();
			for(auto listener : listeners) {
				listener->onStreamPlayerTrackFinish(this, audioURL);
			}
		};
	}

	StreamPlayer::~StreamPlayer() {
		playQueue.cancelAllTasks();
		destroyPlayer();
		destroyPreparedPlayer();
	}



	AVPlayer* StreamPlayer::createPlayer(String audioURL) {
		return [AVPlayer playerWithURL:[NSURL URLWithString:audioURL.toNSString()]];
	}

	void StreamPlayer::setPlayer(AVPlayer* player, String audioURL) {
		std::unique_lock<std::recursive_mutex> lock(playerMutex);
		destroyPlayer();
		this->player = player;
		this->playerAudioURL = audioURL;
		[NSNotificationCenter.defaultCenter addObserver:playerEventHandler selector:@selector(playerItemDidReachEnd:) name:AVPlayerItemDidPlayToEndTimeNotification object:player.currentItem];
		[player addObserver:playerEventHandler forKeyPath:@"timeControlStatus" options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld) context:nil];
	}

	void StreamPlayer::destroyPlayer() {
		std::unique_lock<std::recursive_mutex> lock(playerMutex);
		if(player == nil) {
			return;
		}
		AVPlayer* deadPlayer = player;
		player = nil;
		playerAudioURL.clear();
		[NSNotificationCenter.defaultCenter removeObserver:playerEventHandler name:AVPlayerItemDidPlayToEndTimeNotification object:deadPlayer.currentItem];
		[deadPlayer removeObserver:playerEventHandler forKeyPath:@"timeControlStatus"];
		lock.unlock();
		[deadPlayer pause];
		[deadPlayer replaceCurrentItemWithPlayerItem:nil];
	}

	void StreamPlayer::destroyPreparedPlayer() {
		std::unique_lock<std::recursive_mutex> lock(playerMutex);
		if(preparedPlayer == nil) {
			return;
		}
		//AVPlayer* deadPlayer = preparedPlayer;
		preparedPlayer = nil;
		preparedAudioURL.clear();
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
		return playQueue.run(runOptions, [=](auto task) {
			std::unique_lock<std::recursive_mutex> lock(playerMutex);
			if(preparedAudioURL == audioURL || playerAudioURL == audioURL) {
				return;
			}
			destroyPreparedPlayer();
			preparedPlayer = createPlayer(audioURL);
			preparedAudioURL = audioURL;
			lock.unlock();
			preparedPlayer.muted = YES;
			[preparedPlayer play];
			[preparedPlayer pause];
			preparedPlayer.muted = NO;
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
		return playQueue.run(runOptions, [this,a1=audioURL,a2=options](auto task) -> Generator<void> {
			auto audioURL = a1;
			auto options = a2;
			co_yield setGenResumeQueue(DispatchQueue::main());
			co_yield initialGenNext();
			if(playerAudioURL == audioURL) {
				FGL_ASSERT(player != nil, "player is nil but playerAudioURL is not empty");
				if(player.currentTime.value != 0) {
					bool finished = co_await Promise<bool>([=](auto resolve, auto reject) {
						[player seekToTime:CMTimeMake(0, 1000) completionHandler:^(BOOL finished) {
							resolve(finished);
						}];
					});
					if(!finished) {
						throw std::runtime_error("Failed to reset playback");
					}
					if(player.timeControlStatus != AVPlayerTimeControlStatusPlaying && player.timeControlStatus != AVPlayerTimeControlStatusWaitingToPlayAtSpecifiedRate) {
						[player play];
					}
				}
				co_return;
			} else if(preparedAudioURL == audioURL) {
				std::unique_lock<std::recursive_mutex> lock(playerMutex);
				auto newPlayer = preparedPlayer;
				preparedPlayer = nil;
				preparedAudioURL.clear();
				setPlayer(newPlayer, audioURL);
				lock.unlock();
			} else {
				std::unique_lock<std::recursive_mutex> lock(playerMutex);
				destroyPreparedPlayer();
				setPlayer(createPlayer(audioURL), audioURL);
				lock.unlock();
			}
			co_yield {};
			if(options.beforePlay) {
				options.beforePlay();
			}
			FGL_ASSERT(player != nil, "player is nil after setting up for playback");
			double playerCurrentPos = ((double)player.currentTime.value / (double)player.currentTime.timescale);
			if(playerCurrentPos != options.position) {
				co_await Promise<void>([=](auto resolve, auto reject) {
					[player seekToTime:CMTimeMake(0, 1000) completionHandler:^(BOOL finished) {
						if(!finished) {
							reject(std::runtime_error("Failed to reset playback"));
							return;
						}
						resolve();
					}];
				});
			}
			[player play];
		}).promise;
	}

	Promise<void> StreamPlayer::setPlaying(bool playing) {
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "setPlaying",
			.cancelMatchingTags = true
		};
		return playQueue.run(runOptions, [=](auto task) {
			if(player == nil) {
				return;
			}
			if(playing) {
				[player play];
			} else {
				[player pause];
			}
		}).promise;
	}

	Promise<void> StreamPlayer::seek(double position) {
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "seek",
			.cancelMatchingTags = true
		};
		return playQueue.run(runOptions, [=](auto task) {
			if(player == nil) {
				return Promise<void>::resolve();
			}
			return Promise<void>([=](auto resolve, auto reject) {
				[player seekToTime:CMTimeMake((CMTimeValue)(position * 1000.0), 1000) completionHandler:^(BOOL finished) {
					resolve();
				}];
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
			destroyPlayer();
			destroyPreparedPlayer();
		}).promise;
	}

	StreamPlayer::PlaybackState StreamPlayer::getState() const {
		std::unique_lock<std::recursive_mutex> lock(playerMutex);
		AVPlayer* player = this->player;
		if(player == nil) {
			return {
				.playing = false,
				.position = 0.0,
				.duration = 0.0
			};
		}
		auto currentTime = player.currentTime;
		auto currentItem = player.currentItem;
		auto currentDuration = (currentItem != nil) ? currentItem.duration : CMTimeMake(0, 1000);
		return {
			.playing = (player.timeControlStatus == AVPlayerTimeControlStatusPlaying || player.timeControlStatus == AVPlayerTimeControlStatusWaitingToPlayAtSpecifiedRate),
			.position = (1.0 / (double)currentTime.timescale) * (double)currentTime.value,
			.duration = (1.0 / (double)currentDuration.timescale) * (double)currentDuration.value
		};
	}
}

#endif

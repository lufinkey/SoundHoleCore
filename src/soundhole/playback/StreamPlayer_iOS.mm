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
				listener->onStreamPlayerPlay(this->shared_from_this());
			}
		};
		playerEventHandler.onPause = ^{
			std::unique_lock<std::mutex> lock(listenersMutex);
			auto listeners = this->listeners;
			lock.unlock();
			for(auto listener : listeners) {
				listener->onStreamPlayerPause(this->shared_from_this());
			}
		};
		playerEventHandler.onItemFinish = ^(AVPlayerItem* _Nonnull item) {
			auto audioURL = playerAudioURL;
			if(item != player.currentItem) {
				return;
			}
			std::unique_lock<std::mutex> lock(listenersMutex);
			auto listeners = this->listeners;
			lock.unlock();
			for(auto listener : listeners) {
				listener->onStreamPlayerTrackFinish(this->shared_from_this(), audioURL);
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
		destroyPlayer();
		this->player = player;
		this->playerAudioURL = audioURL;
		[NSNotificationCenter.defaultCenter addObserver:playerEventHandler selector:@selector(playerItemDidReachEnd:) name:AVPlayerItemDidPlayToEndTimeNotification object:player.currentItem];
		[player addObserver:playerEventHandler forKeyPath:@"timeControlStatus" options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld) context:nil];
	}

	void StreamPlayer::destroyPlayer() {
		if(player == nil) {
			return;
		}
		AVPlayer* deadPlayer = player;
		player = nil;
		playerAudioURL.clear();
		[NSNotificationCenter.defaultCenter removeObserver:playerEventHandler name:AVPlayerItemDidPlayToEndTimeNotification object:deadPlayer.currentItem];
		[deadPlayer removeObserver:playerEventHandler forKeyPath:@"timeControlStatus"];
		[deadPlayer pause];
		[deadPlayer replaceCurrentItemWithPlayerItem:nil];
	}

	void StreamPlayer::destroyPreparedPlayer() {
		if(preparedPlayer == nil) {
			return;
		}
		//AVPlayer* deadPlayer = preparedPlayer;
		preparedPlayer = nil;
		preparedAudioURL.clear();
	}



	Promise<void> StreamPlayer::prepare(String audioURL) {
		if(audioURL.empty()) {
			return rejectWith(std::invalid_argument("audioURL cannot be empty"));
		}
		if(preparedAudioURL == audioURL || playerAudioURL == audioURL) {
			return resolveVoid();
		}
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "prepare",
			.cancelMatchingTags = true
		};
		w$<StreamPlayer> weakSelf = shared_from_this();
		return playQueue.run(runOptions, [=](auto task) {
			// ensure strong reference
			auto slf = weakSelf.lock();
			if(!slf) {
				return;
			}
			if(slf->preparedAudioURL == audioURL || slf->playerAudioURL == audioURL) {
				return;
			}
			slf->destroyPreparedPlayer();
			slf->preparedPlayer = slf->createPlayer(audioURL);
			slf->preparedAudioURL = audioURL;
			slf->preparedPlayer.muted = YES;
			[slf->preparedPlayer play];
			[slf->preparedPlayer pause];
			slf->preparedPlayer.muted = NO;
		}).promise;
	}

	Promise<void> StreamPlayer::play(String audioURL, PlayOptions options) {
		if(audioURL.empty()) {
			return rejectWith(std::invalid_argument("audioURL cannot be empty"));
		}
		w$<StreamPlayer> weakSelf = shared_from_this();
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "play",
			.cancelMatchingTags = true
		};
		return playQueue.run(runOptions, coLambda([=](auto task) -> Generator<void> {
			co_yield setGenResumeQueue(DispatchQueue::main());
			co_yield initialGenNext();
			// ensure strong reference
			auto slf = weakSelf.lock();
			if(!slf) {
				co_return;
			}
			// check if audio URL matches
			if(slf->playerAudioURL == audioURL) {
				FGL_ASSERT(slf->player != nil, "player is nil but playerAudioURL is not empty");
				if(slf->player.currentTime.value != 0) {
					bool finished = co_await Promise<bool>([=](auto resolve, auto reject) {
						[slf->player seekToTime:CMTimeMake(0, 1000) completionHandler:^(BOOL finished) {
							resolve(finished);
						}];
					});
					if(!finished) {
						throw std::runtime_error("Failed to reset playback");
					}
					if(slf->player.timeControlStatus != AVPlayerTimeControlStatusPlaying && slf->player.timeControlStatus != AVPlayerTimeControlStatusWaitingToPlayAtSpecifiedRate) {
						[slf->player play];
					}
				}
				co_return;
			} else if(slf->preparedAudioURL == audioURL) {
				auto newPlayer = slf->preparedPlayer;
				slf->preparedPlayer = nil;
				slf->preparedAudioURL.clear();
				slf->setPlayer(newPlayer, audioURL);
			} else {
				slf->destroyPreparedPlayer();
				slf->setPlayer(slf->createPlayer(audioURL), audioURL);
			}
			co_yield {};
			if(options.beforePlay) {
				options.beforePlay();
			}
			FGL_ASSERT(slf->player != nil, "player is nil after setting up for playback");
			double playerCurrentPos = ((double)slf->player.currentTime.value / (double)slf->player.currentTime.timescale);
			if(playerCurrentPos != options.position) {
				co_await Promise<void>([=](auto resolve, auto reject) {
					[slf->player seekToTime:CMTimeMake(0, 1000) completionHandler:^(BOOL finished) {
						if(!finished) {
							reject(std::runtime_error("Failed to reset playback"));
							return;
						}
						resolve();
					}];
				});
			}
			[slf->player play];
		})).promise;
	}

	Promise<void> StreamPlayer::setPlaying(bool playing) {
		w$<StreamPlayer> weakSelf = shared_from_this();
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "setPlaying",
			.cancelMatchingTags = true
		};
		return playQueue.run(runOptions, [=](auto task) {
			auto slf = weakSelf.lock();
			if(!slf) {
				return;
			}
			if(slf->player == nil) {
				return;
			}
			if(playing) {
				[slf->player play];
			} else {
				[slf->player pause];
			}
		}).promise;
	}

	Promise<void> StreamPlayer::seek(double position) {
		w$<StreamPlayer> weakSelf = shared_from_this();
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "seek",
			.cancelMatchingTags = true
		};
		return playQueue.run(runOptions, [=](auto task) -> Promise<void> {
			auto slf = weakSelf.lock();
			if(!slf) {
				return resolveVoid();
			}
			if(slf->player == nil) {
				return resolveVoid();
			}
			return Promise<void>([=](auto resolve, auto reject) {
				[slf->player seekToTime:CMTimeMake((CMTimeValue)(position * 1000.0), 1000) completionHandler:^(BOOL finished) {
					resolve();
				}];
			});
		}).promise;
	}

	Promise<void> StreamPlayer::stop() {
		w$<StreamPlayer> weakSelf = shared_from_this();
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "stop",
			.cancelMatchingTags = true
		};
		playQueue.cancelAllTasks();
		return playQueue.run(runOptions, [=](auto task) {
			auto slf = weakSelf.lock();
			if(!slf) {
				return;
			}
			slf->destroyPlayer();
			slf->destroyPreparedPlayer();
		}).promise;
	}

	StreamPlayer::PlaybackState StreamPlayer::getState() const {
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

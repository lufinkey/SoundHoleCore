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
		//
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
		[deadPlayer pause];
		[deadPlayer replaceCurrentItemWithPlayerItem:nil];
		[NSNotificationCenter.defaultCenter removeObserver:playerEventHandler name:AVPlayerItemDidPlayToEndTimeNotification object:deadPlayer.currentItem];
		[deadPlayer removeObserver:playerEventHandler forKeyPath:@"timeControlStatus"];
	}

	void StreamPlayer::destroyPreparedPlayer() {
		if(preparedPlayer == nil) {
			return;
		}
		AVPlayer* deadPlayer = preparedPlayer;
		preparedPlayer = nil;
		preparedAudioURL.clear();
		[deadPlayer pause];
		[deadPlayer replaceCurrentItemWithPlayerItem:nil];
		[NSNotificationCenter.defaultCenter removeObserver:playerEventHandler name:AVPlayerItemDidPlayToEndTimeNotification object:deadPlayer.currentItem];
		[deadPlayer removeObserver:playerEventHandler forKeyPath:@"timeControlStatus"];
	}



	Promise<void> StreamPlayer::prepare(String audioURL) {
		if(audioURL.empty()) {
			return Promise<void>::reject(std::invalid_argument("audioURL cannot be empty"));
		}
		if(preparedAudioURL == audioURL) {
			return Promise<void>::resolve();
		}
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "prepare",
			.cancelMatchingTags = true
		};
		return playQueue.run(runOptions, [=](auto task) {
			return generate<void>([=](auto yield) {
				if(preparedAudioURL == audioURL) {
					return;
				}
				destroyPreparedPlayer();
				preparedPlayer = createPlayer(audioURL);
				preparedAudioURL = audioURL;
				preparedPlayer.muted = YES;
				[preparedPlayer play];
				[preparedPlayer pause];
				preparedPlayer.muted = NO;
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
		return playQueue.run(runOptions, [=](auto task) {
			return generate<void>([=](auto yield) {
				if(playerAudioURL == audioURL) {
					FGL_ASSERT(player != nil, "player is nil but playerAudioURL is not empty");
					await(Promise<void>([=](auto resolve, auto reject) {
						if(player.currentTime.value != 0) {
							[player seekToTime:CMTimeMake(0, 1000) completionHandler:^(BOOL finished) {
								if(!finished) {
									reject(std::runtime_error("Failed to reset playback"));
									return;
								}
								if(player.timeControlStatus != AVPlayerTimeControlStatusPlaying && player.timeControlStatus != AVPlayerTimeControlStatusWaitingToPlayAtSpecifiedRate) {
									[player play];
								}
								resolve();
							}];
						}
					}));
					return;
				} else if(preparedAudioURL == audioURL) {
					auto newPlayer = preparedPlayer;
					preparedPlayer = nil;
					preparedAudioURL.clear();
					setPlayer(newPlayer, audioURL);
				} else {
					destroyPreparedPlayer();
					setPlayer(createPlayer(audioURL), audioURL);
				}
				yield();
				if(options.beforePlay) {
					options.beforePlay();
				}
				FGL_ASSERT(player != nil, "player is nil after setting up for playback");
				double playerCurrentPos = ((double)player.currentTime.value / (double)player.currentTime.timescale);
				if(playerCurrentPos != options.position) {
					await(Promise<void>([=](auto resolve, auto reject) {
						[player seekToTime:CMTimeMake(0, 1000) completionHandler:^(BOOL finished) {
							if(!finished) {
								reject(std::runtime_error("Failed to reset playback"));
								return;
							}
							resolve();
						}];
					}));
				}
				[player play];
				// TODO emit play event
			});
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
			// TODO emit play/pause event
		}).promise;
	}

	Promise<void> StreamPlayer::seek(double position) {
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "seek",
			.cancelMatchingTags = true
		};
		return playQueue.run(runOptions, [=](auto task) {
			if(player == nil) {
				return;
			}
			await(Promise<void>([=](auto resolve, auto reject) {
				[player seekToTime:CMTimeMake((CMTimeValue)(position * 1000.0), 1000) completionHandler:^(BOOL finished) {
					resolve();
				}];
			}));
		}).promise;
	}

	Promise<void> StreamPlayer::stop() {
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "stop",
			.cancelMatchingTags = true
		};
		playQueue.cancelAllTasks();
		return playQueue.run(runOptions, [=](auto task) {
			destroyPlayer();
			destroyPreparedPlayer();
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
			.duration = (1.0 / (double)currentDuration.timescale) * (double)currentDuration.value,
		};
	}
}

#endif

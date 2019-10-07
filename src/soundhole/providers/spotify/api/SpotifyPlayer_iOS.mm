//
//  SpotifyPlayer_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 9/22/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SpotifyPlayer.hpp"
#include "SpotifyError.hpp"

#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <Foundation/Foundation.h>

namespace sh {
	SpotifyPlayer::SpotifyPlayer()
	: auth(nullptr), player(SPTAudioStreamingController.sharedInstance),
	playerEventHandler([[SpotifyPlayerEventHandler alloc] init]),
	starting(false), loggingIn(false), loggedIn(false), loggingOut(false), renewingSession(false) {
		playerEventHandler.onError = ^(NSError* error) {
			if(loggingIn) {
				onStreamingLoginError(SpotifyError(error));
			} else {
				onStreamingError(SpotifyError(error));
			}
		};
		playerEventHandler.onLogin = ^() {
			onStreamingLogin();
		};
		playerEventHandler.onLogout = ^() {
			onStreamingLogout();
		};
		playerEventHandler.onTemporaryConnectionError = ^() {
			// TODO handle temporary connection error
		};
		playerEventHandler.onMessage = ^(NSString* message) {
			// TODO handle player message
		};
		playerEventHandler.onDisconnect = ^() {
			// TODO handle disconnect
		};
		playerEventHandler.onReconnect = ^() {
			// TODO handle reconnect
		};
		playerEventHandler.onPlaybackEvent = ^(SpPlaybackEvent event) {
			// TODO handle playback event
		};
	}

	SpotifyPlayer::~SpotifyPlayer() {
		//
	}
	
	
	
	Promise<void> SpotifyPlayer::start() {
		std::unique_lock<std::recursive_mutex> lock(startMutex);
		if(starting) {
			return Promise<void>([=](auto resolve, auto reject) {
				startCallbacks.pushBack({ resolve, reject });
			});
		}
		else if(player.initialized) {
			return Promise<void>::resolve();
		}
		starting = true;
		lock.unlock();
		
		return Promise<void>([=](auto mainResolve, auto mainReject) {
			Promise<void>([=](auto resolve, auto reject) {
				if(auth == nullptr) {
					reject(SpotifyError(SpotifyError::Code::BAD_PARAMETERS, "auth has not been assigned to player"));
					return;
				}
				if(player.initialized) {
					resolve();
					return;
				}
				auto& options = auth->getOptions();
				NSError* initError = nil;
				bool initialized = [player startWithClientId:options.clientId.toNSString() error:&initError];
				if(!initialized) {
					reject(SpotifyError(initError));
					return;
				}
				player.delegate = playerEventHandler;
				player.playbackDelegate = playerEventHandler;
				resolve();
			}).then([=]() {
				std::unique_lock<std::recursive_mutex> lock(startMutex);
				starting = false;
				LinkedList<WaitCallback> callbacks;
				callbacks.swap(startCallbacks);
				lock.unlock();
				
				mainResolve();
				for(auto& callback : callbacks) {
					callback.resolve();
				}
			}).except([=](std::exception_ptr errorPtr) {
				std::unique_lock<std::recursive_mutex> lock(startMutex);
				starting = false;
				LinkedList<WaitCallback> callbacks;
				callbacks.swap(startCallbacks);
				lock.unlock();
				
				mainReject(errorPtr);
				for(auto& callback : callbacks) {
					callback.reject(errorPtr);
				}
			});
		});
	}
	
	void SpotifyPlayer::stop() {
		NSError* error = nil;
		BOOL stopped = [player stopWithError:&error];
		if(!stopped) {
			NSLog(@"Error stopping SpotifyPlayer: %@", error);
		}
	}
	
	bool SpotifyPlayer::isStarted() const {
		return (bool)player.initialized;
	}
	
	bool SpotifyPlayer::isPlayerLoggedIn() const {
		return (bool)player.loggedIn;
	}

	void SpotifyPlayer::applyAuthToken(String accessToken) {
		[player loginWithAccessToken:accessToken.toNSString()];
	}
	
	Promise<void> SpotifyPlayer::logout() {
		std::unique_lock<std::mutex> lock(loginMutex);
		if(!loggedIn && !isPlayerLoggedIn()) {
			return Promise<void>::resolve();
		}
		return Promise<void>([&](auto resolve, auto reject) {
			logoutCallbacks.pushBack({ resolve, reject });
			if(!loggingOut) {
				loggingOut = true;
				lock.unlock();
				[player logout];
			}
		});
	}
	
	
	
	
	Promise<void> SpotifyPlayer::playURI(String uri, PlayOptions options) {
		return prepareForCall().then([=]() -> Promise<void> {
			return Promise<void>([&](auto resolve, auto reject) {
				[player playSpotifyURI:uri.toNSString() startingWithIndex:(NSUInteger)options.index startingWithPosition:(NSTimeInterval)(options.position*1000.0) callback:^(NSError* error) {
					if(error != nil) {
						reject(SpotifyError(error));
					} else {
						resolve();
					}
				}];
			});
		});
	}
	
	Promise<void> SpotifyPlayer::queueURI(String uri) {
		return prepareForCall().then([=]() -> Promise<void> {
			return Promise<void>([&](auto resolve, auto reject) {
				[player queueSpotifyURI:uri.toNSString() callback:^(NSError* error) {
					if(error != nil) {
						reject(SpotifyError(error));
					} else {
						resolve();
					}
				}];
			});
		});
	}
	
	Promise<void> SpotifyPlayer::skipToNext() {
		return prepareForCall().then([=]() -> Promise<void> {
			return Promise<void>([&](auto resolve, auto reject) {
				[player skipNext:^(NSError* error) {
					if(error != nil) {
						reject(SpotifyError(error));
					} else {
						resolve();
					}
				}];
			});
		});
	}
	
	Promise<void> SpotifyPlayer::skipToPrevious() {
		return prepareForCall().then([=]() -> Promise<void> {
			return Promise<void>([&](auto resolve, auto reject) {
				[player skipPrevious:^(NSError* error) {
					if(error != nil) {
						reject(SpotifyError(error));
					} else {
						resolve();
					}
				}];
			});
		});
	}
	
	Promise<void> SpotifyPlayer::seek(double position) {
		return prepareForCall((position != 0.0)).then([=]() -> Promise<void> {
			return Promise<void>([&](auto resolve, auto reject) {
				[player seekTo:(NSTimeInterval)position callback:^(NSError* error) {
					if(error != nil) {
						reject(SpotifyError(error));
					} else {
						resolve();
					}
				}];
			});
		});
	}
	
	Promise<void> SpotifyPlayer::setPlaying(bool playing) {
		return prepareForCall(playing).then([=]() -> Promise<void> {
			return Promise<void>([&](auto resolve, auto reject) {
				[player setIsPlaying:(BOOL)playing callback:^(NSError* error) {
					if(error != nil) {
						reject(SpotifyError(error));
					} else {
						resolve();
					}
				}];
			});
		});
	}
	
	Promise<void> SpotifyPlayer::setShuffling(bool shuffling) {
		return Promise<void>([&](auto resolve, auto reject) {
			[player setShuffle:(BOOL)shuffling callback:^(NSError* error) {
				if(error != nil) {
					reject(SpotifyError(error));
				} else {
					resolve();
				}
			}];
		});
	}
	
	Promise<void> SpotifyPlayer::setRepeating(bool repeating) {
		return Promise<void>([&](auto resolve, auto reject) {
			[player setRepeat:(repeating ? SPTRepeatContext : SPTRepeatOff) callback:^(NSError* error) {
				if(error != nil) {
					reject(SpotifyError(error));
				} else {
					resolve();
				}
			}];
		});
	}
	
	SpotifyPlayer::State SpotifyPlayer::getState() const {
		return stateFromSPTPlaybackState(player.playbackState);
	}
	
	SpotifyPlayer::Metadata SpotifyPlayer::getMetadata() const {
		return metadataFromSPTPlaybackMetadata(player.metadata);
	}
	
	
	
	
	SpotifyPlayer::State SpotifyPlayer::stateFromSPTPlaybackState(SPTPlaybackState* state) {
		return State{
			.playing = (bool)state.isPlaying,
			.repeating = (bool)state.isRepeating,
			.shuffling = (bool)state.isShuffling,
			.activeDevice = (bool)state.isActiveDevice,
			.position = (double)state.position
		};
	}
	
	SpotifyPlayer::Track SpotifyPlayer::trackFromSPTPlaybackTrack(SPTPlaybackTrack* track) {
		return Track{
			.uri = String(track.uri),
			.name = String(track.name),
			.artistURI = String(track.artistUri),
			.artistName = String(track.artistName),
			.albumURI = String(track.albumUri),
			.albumName = String(track.albumName),
			.albumCoverArtURL = ((track.albumCoverArtURL != nil) ? String(track.albumCoverArtURL) : Optional<String>()),
			.duration = (double)track.duration,
			.indexInContext = (size_t)track.indexInContext,
			.contextURI = String(track.playbackSourceUri),
            .contextName = String(track.playbackSourceName)
		};
	}
	
	SpotifyPlayer::Metadata SpotifyPlayer::metadataFromSPTPlaybackMetadata(SPTPlaybackMetadata* metadata) {
		return Metadata{
			.previousTrack = (metadata.prevTrack != nil) ? trackFromSPTPlaybackTrack(metadata.prevTrack) : Optional<Track>(),
			.currentTrack = (metadata.currentTrack != nil) ? trackFromSPTPlaybackTrack(metadata.currentTrack) : Optional<Track>(),
			.nextTrack = (metadata.nextTrack != nil) ? trackFromSPTPlaybackTrack(metadata.nextTrack) : Optional<Track>()
		};
	}
	
	
	
	
	void SpotifyPlayer::restoreSavedPlayerState(const SavedPlayerState& state) {
		[player setShuffle:(BOOL)state.shuffling callback:^(NSError*) {}];
		[player setRepeat:(state.repeating ? SPTRepeatContext : SPTRepeatOff) callback:^(NSError*) {}];
		[player playSpotifyURI:state.uri.toNSString() startingWithIndex:(NSUInteger)state.index startingWithPosition:(NSTimeInterval)state.position callback:^(NSError* error) {
			if(error != nil) {
				NSLog(@"failed to play uri after unexpected pause: %@", error);
			}
		}];
		[player setIsPlaying:(BOOL)state.playing callback:^(NSError* error) {
			if(error != nil) {
				NSLog(@"failed to set player playing after unexpected pause: %@", error);
			}
		}];
	}
	
	SpotifyPlayer::SavedPlayerState SpotifyPlayer::getPlayerState() {
		SPTPlaybackMetadata* metadata = player.metadata;
		SPTPlaybackState* state = player.playbackState;
		if(metadata != nil && metadata.currentTrack != nil) {
			SPTPlaybackTrack* currentTrack = metadata.currentTrack;
			return SavedPlayerState{
				.uri = (currentTrack.playbackSourceUri != nil) ? String(currentTrack.playbackSourceUri) : "",
				.index = (currentTrack.playbackSourceUri != nil) ? (size_t)currentTrack.indexInContext : 0,
				.position = (double)state.position,
				.shuffling = (bool)state.isShuffling,
				.repeating = (bool)state.isRepeating,
				.playing = (bool)state.isPlaying
			};
		} else {
			return SavedPlayerState{
				.uri = "",
				.index = 0,
				.position = 0.0,
				.shuffling = (bool)state.isShuffling,
				.repeating = (bool)state.isRepeating,
				.playing = false
			};
		}
	}
}

#endif

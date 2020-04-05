//
//  StreamPlaybackProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/14/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "StreamPlaybackProvider.hpp"
#include <soundhole/utils/SoundHoleError.hpp>

namespace sh {
	StreamPlaybackProvider::StreamPlaybackProvider(StreamPlayer* streamPlayer)
	: player(streamPlayer) {
		player->addListener(this);
	}

	StreamPlaybackProvider::~StreamPlaybackProvider() {
		player->removeListener(this);
	}




	bool StreamPlaybackProvider::usesPublicAudioStreams() const {
		return true;
	}
	
	Promise<void> StreamPlaybackProvider::prepare($<Track> track) {
		std::unique_lock<std::mutex> lock(currentTrackMutex);
		if(currentTrack != nullptr && currentTrack->uri() == track->uri()) {
			return Promise<void>::resolve();
		}
		lock.unlock();
		return prepareQueue.run({.cancelAll=true}, [=](auto task) {
			return generate_items<void>({
				[=]() {
					return Promise<void>::resolve().then([=]() {
						if(track->audioSources().has_value()) {
							return Promise<void>::resolve();
						}
						return track->fetchDataIfNeeded();
					});
				},
				[=]() {
					return Promise<void>::resolve().then([=]() {
						if(!track->audioSources().has_value() || track->audioSources()->size() == 0) {
							return Promise<void>::reject(
								SoundHoleError(SoundHoleError::Code::STREAM_UNAVAILABLE, "No audio streams available for track with uri "+track->uri()+" ("+track->name()+")"));
						}
						auto audioSource = track->findAudioSource();
						if(!audioSource) {
							return Promise<void>::reject(
								SoundHoleError(SoundHoleError(SoundHoleError::Code::STREAM_UNAVAILABLE, "Unable to find audio stream for track with uri "+track->uri()+" ("+track->name()+")")));
						}
						return player->prepare(audioSource->url);
					});
				}
			});
		}).promise;
	}

	Promise<void> StreamPlaybackProvider::play($<Track> track, double position) {
		return playQueue.run({.cancelAll=true}, [=](auto task) {
			return generate_items<void>({
				[=]() {
					return track->fetchDataIfNeeded();
				},
				[=]() {
					if(!track->audioSources().has_value() || track->audioSources()->size() == 0) {
						return Promise<void>::reject(
							SoundHoleError(SoundHoleError::Code::STREAM_UNAVAILABLE, "No audio streams available for track with uri "+track->uri()+" ("+track->name()+")"));
					}
					auto audioSource = track->findAudioSource();
					if(!audioSource) {
						return Promise<void>::reject(
							SoundHoleError(SoundHoleError(SoundHoleError::Code::STREAM_UNAVAILABLE, "Unable to find audio stream for track with uri "+track->uri()+" ("+track->name()+")")));
					}
					return player->play(audioSource->url, {
						.position=position,
						.beforePlay=[=]() {
							std::unique_lock<std::mutex> lock(currentTrackMutex);
							this->currentTrack = track;
							this->currentTrackAudioURL = audioSource->url;
							lock.unlock();
							callListenerEvent(&EventListener::onMediaPlaybackProviderMetadataChange, this);
						}
					});
				}
			});
		}).promise;
	}

	Promise<void> StreamPlaybackProvider::setPlaying(bool playing) {
		return player->setPlaying(playing);
	}

	void StreamPlaybackProvider::stop() {
		playQueue.cancelAllTasks();
		prepareQueue.cancelAllTasks();
		player->stop();
		std::unique_lock<std::mutex> lock(currentTrackMutex);
		currentTrack = nullptr;
		currentTrackAudioURL.clear();
		lock.unlock();
	}

	Promise<void> StreamPlaybackProvider::seek(double position) {
		return player->seek(position);
	}
	
	MediaPlaybackProvider::State StreamPlaybackProvider::state() const {
		auto playerState = player->getState();
		return State{
			.playing=playerState.playing,
			.position=playerState.position,
			.shuffling=false,
			.repeating=false
		};
	}

	MediaPlaybackProvider::Metadata StreamPlaybackProvider::metadata() const {
		std::unique_lock<std::mutex> lock(currentTrackMutex);
		return Metadata{
			.previousTrack=nullptr,
			.currentTrack=currentTrack,
			.nextTrack=nullptr
		};
	}



#pragma mark StreamPlayer::Listener
	
	void StreamPlaybackProvider::onStreamPlayerPlay(StreamPlayer* player) {
		std::unique_lock<std::mutex> lock(currentTrackMutex);
		if(currentTrackAudioURL.empty() || player->getAudioURL() != currentTrackAudioURL) {
			currentTrack = nullptr;
			currentTrackAudioURL.clear();
			return;
		}
		lock.unlock();
		callListenerEvent(&EventListener::onMediaPlaybackProviderPlay, this);
	}

	void StreamPlaybackProvider::onStreamPlayerPause(StreamPlayer* player) {
		std::unique_lock<std::mutex> lock(currentTrackMutex);
		if(currentTrackAudioURL.empty() || player->getAudioURL() != currentTrackAudioURL) {
			currentTrack = nullptr;
			currentTrackAudioURL.clear();
			return;
		}
		lock.unlock();
		callListenerEvent(&EventListener::onMediaPlaybackProviderPause, this);
	}

	void StreamPlaybackProvider::onStreamPlayerTrackFinish(StreamPlayer* player, String audioURL) {
		std::unique_lock<std::mutex> lock(currentTrackMutex);
		if(currentTrackAudioURL.empty() || audioURL != currentTrackAudioURL) {
			currentTrack = nullptr;
			currentTrackAudioURL.clear();
			return;
		}
		lock.unlock();
		callListenerEvent(&EventListener::onMediaPlaybackProviderTrackFinish, this);
	}
}

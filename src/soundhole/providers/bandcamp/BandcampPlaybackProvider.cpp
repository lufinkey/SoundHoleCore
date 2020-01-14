//
//  BandcampPlaybackProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/13/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "BandcampPlaybackProvider.hpp"
#include <soundhole/utils/SoundHoleError.hpp>

namespace sh {
	BandcampPlaybackProvider::BandcampPlaybackProvider(BandcampProvider* provider, StreamPlayer* streamPlayer)
	: provider(provider), player(streamPlayer) {
		streamPlayer->addListener(this);
	}

	BandcampPlaybackProvider::~BandcampPlaybackProvider() {
		player->removeListener(this);
	}




	bool BandcampPlaybackProvider::usesPublicAudioStreams() const {
		return true;
	}
	
	Promise<void> BandcampPlaybackProvider::prepare($<Track> track) {
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
						return track->fetchMissingDataIfNeeded();
					});
				},
				[=]() {
					return Promise<void>::resolve().then([=]() {
						if(!track->audioSources().has_value() || track->audioSources()->size() == 0) {
							return Promise<void>::reject(
								SoundHoleError(SoundHoleError::Code::STREAM_UNAVAILABLE, "No bandcamp audio streams available for track "+track->name()));
						}
						auto audioSource = track->findAudioSource();
						if(!audioSource) {
							return Promise<void>::reject(
								SoundHoleError(SoundHoleError(SoundHoleError::Code::STREAM_UNAVAILABLE, "Unable to find bandcamp audio stream for track "+track->name())));
						}
						return player->prepare(audioSource->url);
					});
				}
			});
		}).promise;
	}

	Promise<void> BandcampPlaybackProvider::play($<Track> track, double position) {
		return playQueue.run({.cancelAll=true}, [=](auto task) {
			return generate_items<void>({
				[=]() {
					return track->fetchMissingDataIfNeeded();
				},
				[=]() {
					if(!track->audioSources().has_value() || track->audioSources()->size() == 0) {
						return Promise<void>::reject(
							SoundHoleError(SoundHoleError::Code::STREAM_UNAVAILABLE, "No bandcamp audio streams available for track "+track->name()));
					}
					auto audioSource = track->findAudioSource();
					if(!audioSource) {
						return Promise<void>::reject(
							SoundHoleError(SoundHoleError(SoundHoleError::Code::STREAM_UNAVAILABLE, "Unable to find bandcamp audio stream for track "+track->name())));
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

	Promise<void> BandcampPlaybackProvider::setPlaying(bool playing) {
		return player->setPlaying(playing);
	}

	void BandcampPlaybackProvider::stop() {
		playQueue.cancelAllTasks();
		prepareQueue.cancelAllTasks();
		player->stop();
		std::unique_lock<std::mutex> lock(currentTrackMutex);
		currentTrack = nullptr;
		currentTrackAudioURL.clear();
		lock.unlock();
	}

	Promise<void> BandcampPlaybackProvider::seek(double position) {
		return player->seek(position);
	}
	
	MediaPlaybackProvider::State BandcampPlaybackProvider::state() const {
		auto playerState = player->getState();
		return State{
			.playing=playerState.playing,
			.position=playerState.position,
			.shuffling=false,
			.repeating=false
		};
	}

	MediaPlaybackProvider::Metadata BandcampPlaybackProvider::metadata() const {
		std::unique_lock<std::mutex> lock(currentTrackMutex);
		return Metadata{
			.previousTrack=nullptr,
			.currentTrack=currentTrack,
			.nextTrack=nullptr
		};
	}



#pragma mark StreamPlayer::Listener
	
	void BandcampPlaybackProvider::onStreamPlayerPlay(StreamPlayer* player) {
		std::unique_lock<std::mutex> lock(currentTrackMutex);
		if(currentTrackAudioURL.empty() || player->getAudioURL() != currentTrackAudioURL) {
			currentTrack = nullptr;
			currentTrackAudioURL.clear();
			return;
		}
		lock.unlock();
		callListenerEvent(&EventListener::onMediaPlaybackProviderPlay, this);
	}

	void BandcampPlaybackProvider::onStreamPlayerPause(StreamPlayer* player) {
		std::unique_lock<std::mutex> lock(currentTrackMutex);
		if(currentTrackAudioURL.empty() || player->getAudioURL() != currentTrackAudioURL) {
			currentTrack = nullptr;
			currentTrackAudioURL.clear();
			return;
		}
		lock.unlock();
		callListenerEvent(&EventListener::onMediaPlaybackProviderPause, this);
	}

	void BandcampPlaybackProvider::onStreamPlayerTrackFinish(StreamPlayer* player, String audioURL) {
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

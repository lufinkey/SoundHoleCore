//
//  Player.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 2/9/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "Player.hpp"
#include <soundhole/utils/Utils.hpp>

namespace sh {
	$<Player> Player::new$(Options options) {
		return fgl::new$<Player>(options);
	}

	Player::Player(Options options)
	: options(options),
	organizer(PlaybackOrganizer::new$({.delegate=this})),
	mediaProvider(nullptr),
	preparedMediaProvider(nullptr) {
		organizer->addEventListener(this);
	}

	Player::~Player() {
		organizer->removeEventListener(this);
		organizer->stop();
		setMediaProvider(nullptr);
		stopPlayerStateInterval();
	}



	void Player::addEventListener(EventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		listeners.pushBack(listener);
	}

	void Player::removeEventListener(EventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		listeners.removeLastEqual(listener);
	}



	Promise<void> Player::play($<Track> track) {
		return organizer->play(track);
	}

	Promise<void> Player::play($<TrackCollectionItem> item) {
		return organizer->play(item);
	}

	Promise<void> Player::play($<QueueItem> queueItem) {
		return organizer->play(queueItem);
	}

	Promise<void> Player::play(ItemVariant item) {
		return organizer->play(item);
	}

	Promise<void> Player::playAtQueueIndex(size_t index) {
		auto item = organizer->getQueueItem(index);
		return organizer->play(item);
	}

	Promise<void> Player::seek(double position) {
		if(mediaProvider == nullptr) {
			return Promise<void>::resolve();
		}
		auto playbackProvider = mediaProvider->player();
		if(playbackProvider == nullptr) {
			return Promise<void>::resolve();
		}
		return playbackProvider->seek(position);
	}

	Promise<bool> Player::skipToPrevious() {
		return organizer->previous();
	}

	Promise<bool> Player::skipToNext() {
		return organizer->next();
	}



	$<QueueItem> Player::addToQueue($<Track> track) {
		bool queueWasEmpty = (organizer->getQueueLength() == 0);
		auto queueItem = organizer->addToQueue(track);
		saveInBackground({.includeMetadata=true});
		// if queue was empty and we're not playing anything, attempt to play queued item
		if(queueWasEmpty && !this->playingTrack && !playQueue.getTaskWithTag("play")) {
			w$<Player> weakSelf = shared_from_this();
			playQueue.run({.tag="play"}, [=](auto task) {
				return generate_items<void>({
					[=]() {
						// wait till end of frame before trying
						return Promise<void>([=](auto resolve, auto reject) {
							getDefaultPromiseQueue()->async([=]() {
								resolve();
							});
						});
					},
					[=]() {
						auto self = weakSelf.lock();
						if(!self) {
							return Promise<void>::resolve();
						}
						if(self->playingTrack || self->organizer->getQueueLength() == 0) {
							return Promise<void>::resolve();
						}
						auto queueItem = self->organizer->getQueueItem(0);
						return self->organizer->play(queueItem);
					}
				});
			}).promise.except([=](std::exception_ptr error) {
				// error
				printf("Error while player was auto-starting from queue: %s\n", utils::getExceptionDetails(error).c_str());
			});
		}
		return queueItem;
	}

	void Player::removeFromQueue($<QueueItem> queueItem) {
		this->organizer->removeFromQueue(queueItem);
		saveInBackground({.includeMetadata=true});
	}



	Promise<void> Player::setPlaying(bool playing) {
		// if there is a track waiting to be resumed from save, resume it
		auto resumableProgress = this->resumableProgress;
		if(resumableProgress && mediaProvider == nullptr && playing) {
			auto currentItem = organizer->getCurrentItem();
			auto currentTrack = PlaybackOrganizer::trackFromItem(currentItem);
			if(currentTrack && resumableProgress->uri == currentTrack->uri() && resumableProgress->providerName == currentTrack->mediaProvider()->name()) {
				return organizer->play(currentItem);
			}
		}
		// if there is no playback provider, return without doing anything
		auto playbackProvider = (mediaProvider != nullptr) ? mediaProvider->player() : nullptr;
		if(playbackProvider == nullptr) {
			return Promise<void>::resolve();
		}
		// play from playback provider
		return playbackProvider->setPlaying(playing);
	}

	void Player::setShuffling(bool shuffling) {
		this->organizer->setShuffling(shuffling);
		// emit state change event
		w$<Player> weakSelf = shared_from_this();
		Promise<void>::resolve().then([=]() {
			auto self = weakSelf.lock();
			if(!self) {
				return;
			}
			callPlayerListenerEvent(&EventListener::onPlayerStateChange, self, createEvent());
		});
	}



	Player::Metadata Player::metadata() {
		auto previousTrackPromise = organizer->getPreviousTrack();
		$<Track> previousTrack;
		if(previousTrackPromise.isComplete()) {
			previousTrack = previousTrackPromise.get();
		}
		auto nextTrackPromise = organizer->getNextTrack();
		$<Track> nextTrack;
		if(nextTrackPromise.isComplete()) {
			nextTrack = nextTrackPromise.get();
		}
		auto currentTrack = playingTrack ? playingTrack : organizer->getCurrentTrack();
		return Metadata{
			.previousTrack = previousTrack,
			.currentTrack = currentTrack,
			.nextTrack = nextTrack
		};
	}

	Player::State Player::state() const {
		auto playbackState = providerPlaybackState;
		double position = playbackState ? playbackState->position : 0.0;
		auto resumableProgress = this->resumableProgress;
		if(resumableProgress) {
			auto currentTrack = organizer->getCurrentTrack();
			if(currentTrack && resumableProgress->uri == currentTrack->uri() && resumableProgress->providerName == currentTrack->mediaProvider()->name()) {
				position = resumableProgress->position;
			}
		}
		return State{
			.playing = playbackState ? playbackState->playing : false,
			.position = position,
			.shuffling = organizer->isShuffling()
		};
	}
	
	Player::ItemVariant Player::currentItem() const {
		auto organizerItem = organizer->getCurrentItem();
		auto organizerTrack = PlaybackOrganizer::trackFromItem(organizerItem);
		if(playingTrack && (!organizerTrack || playingTrack->uri() != organizerTrack->uri())) {
			return playingTrack;
		}
		return organizerItem;
	}
	
	$<TrackCollection> Player::context() const {
		return organizer->getContext();
	}

	Optional<size_t> Player::contextIndex() const {
		return organizer->getContextIndex();
	}
	
	LinkedList<$<QueueItem>> Player::queueItems() const {
		return organizer->getQueue();
	}



	Promise<void> Player::prepareTrack($<Track> track) {
		auto runOptions = AsyncQueue::RunOptions{
			.tag="prepare",
			.cancelMatchingTags=true
		};
		w$<Player> weakSelf = shared_from_this();
		return playQueue.run(runOptions, [=](auto task) {
			auto self = weakSelf.lock();
			if(!self) {
				return Promise<void>::resolve();
			}
			auto provider = track->mediaProvider();
			auto playbackProvider = provider->player();
			auto preparedPlaybackProvider = (self->preparedMediaProvider != nullptr) ? self->preparedMediaProvider->player() : nullptr;
			if(preparedPlaybackProvider != nullptr && preparedPlaybackProvider != playbackProvider) {
				self->preparedMediaProvider = nullptr;
			}
			return playbackProvider->prepare(track).then([=]() {
				self->preparedMediaProvider = provider;
			});
		}).promise;
	}

	Promise<void> Player::playTrack($<Track> track) {
		auto runOptions = AsyncQueue::RunOptions{
			.tag="play",
			.cancelMatchingTags=true
		};
		w$<Player> weakSelf = shared_from_this();
		return playQueue.run(runOptions, [=](auto task) {
			auto self = weakSelf.lock();
			if(!self) {
				return Promise<void>::resolve();
			}
			auto provider = track->mediaProvider();
			auto playbackProvider = provider->player();
			auto preparedPlaybackProvider = (self->preparedMediaProvider != nullptr) ? self->preparedMediaProvider->player() : nullptr;
			if(preparedPlaybackProvider != nullptr && preparedPlaybackProvider != playbackProvider) {
				preparedMediaProvider = nullptr;
			}
			auto currentPlaybackProvider = (self->mediaProvider != nullptr) ? self->mediaProvider->player() : nullptr;
			if(currentPlaybackProvider != nullptr && currentPlaybackProvider != playbackProvider) {
				playbackProvider->stop();
			}
			setMediaProvider(provider);
			double position = 0;
			auto resumableProgress = self->resumableProgress;
			if(resumableProgress) {
				if(resumableProgress->uri == track->uri() && resumableProgress->providerName == track->mediaProvider()->name()) {
					position = resumableProgress->position;
				} else {
					resumableProgress = std::nullopt;
				}
			}
			return playbackProvider->play(track, position).then([=]() {
				this->resumableProgress = std::nullopt;
				saveInBackground({.includeMetadata=true});
			});
		}).promise;
	}



	void Player::setMediaProvider(MediaProvider* provider) {
		if (provider == this->mediaProvider) {
			return;
		}
		auto playbackProvider = provider->player();
		auto oldPlaybackProvider = (this->mediaProvider != nullptr) ? this->mediaProvider->player() : nullptr;
		if (oldPlaybackProvider != nullptr) {
			oldPlaybackProvider->removeEventListener(this);
			stopPlayerStateInterval();
		}
		this->mediaProvider = provider;
		if (playbackProvider != nullptr) {
			playbackProvider->addEventListener(this);
			if(playbackProvider->state().playing) {
				startPlayerStateInterval();
			}
		}
	}
	
	void Player::startPlayerStateInterval() {
		if(!providerPlayerStateTimer) {
			providerPlayerStateTimer = Timer::withInterval(std::chrono::milliseconds(100), getDefaultPromiseQueue(), [=]() {
				onPlayerStateInterval();
			});
			onPlayerStateInterval();
		}
	}

	void Player::stopPlayerStateInterval() {
		if(providerPlayerStateTimer) {
			providerPlayerStateTimer->cancel();
			providerPlayerStateTimer = nullptr;
		}
	}

	void Player::onPlayerStateInterval() {
		auto self = shared_from_this();
		
		auto playbackProvider = (this->mediaProvider != nullptr) ? this->mediaProvider->player() : nullptr;
		if(playbackProvider == nullptr) {
			return;
		}
		auto playbackState = playbackProvider->state();
		this->providerPlaybackState = playbackState;
		
		// TODO compare state with previous state
		// emit state change event
		callPlayerListenerEvent(&EventListener::onPlayerStateChange, self, createEvent());
		
		// check if next track needs preparing
		auto metadata = self->metadata();
		if(metadata.currentTrack && playbackState.playing
		   && (playbackState.position + options.nextTrackPreloadTime) >= metadata.currentTrack->duration().value_or(0.0)) {
			// prepare next track
			organizer->prepareNextIfNeeded().except([=](std::exception_ptr error) {
				// error
				printf("Error preparing next track: %s\n", utils::getExceptionDetails(error).c_str());
			});
		}
		// save player state if needed
		auto currentTime = std::chrono::steady_clock::now();
		if(!lastSaveTime || (currentTime - lastSaveTime.value()) >= std::chrono::milliseconds((size_t)(options.progressSaveInterval * 1000))) {
			lastSaveTime = currentTime;
			saveInBackground({.includeMetadata=false});
		}
	}



	Promise<void> Player::onPlaybackOrganizerPrepareTrack($<PlaybackOrganizer> organizer, $<Track> track) {
		return prepareTrack(track);
	}

	Promise<void> Player::onPlaybackOrganizerPlayTrack($<PlaybackOrganizer> organizer, $<Track> track) {
		return playTrack(track);
	}

	void Player::onPlaybackOrganizerTrackChange($<PlaybackOrganizer> organizer) {
		auto self = shared_from_this();
		
		auto prevTrackPromise = organizer->getPreviousTrack();
		auto nextTrackPromise = organizer->getNextTrack();
		
		// emit metadata change event
		callPlayerListenerEvent(&EventListener::onPlayerMetadataChange, self, createEvent());
		
		if(!prevTrackPromise.isComplete()) {
			w$<Player> weakSelf = shared_from_this();
			prevTrackPromise.then([=]($<Track> track) {
				auto self = weakSelf.lock();
				if(!self) {
					return;
				}
				// emit metadata change event
				callPlayerListenerEvent(&EventListener::onPlayerMetadataChange, self, createEvent());
			}).except([=](std::exception_ptr error) {
				printf("Unable to load previous track from organizer: %s\n", utils::getExceptionDetails(error).c_str());
			});
		}
		if(!nextTrackPromise.isComplete()) {
			w$<Player> weakSelf = shared_from_this();
			nextTrackPromise.then([=]($<Track> track) {
				auto self = weakSelf.lock();
				if(!self) {
					return;
				}
				// emit metadata change event
				callPlayerListenerEvent(&EventListener::onPlayerMetadataChange, self, createEvent());
			}).except([=](std::exception_ptr error) {
				printf("Unable to load next track from organizer: %s\n", utils::getExceptionDetails(error).c_str());
			});
		}
	}

	void Player::onPlaybackOrganizerQueueChange($<PlaybackOrganizer> organizer) {
		auto self = shared_from_this();
		// emit queue change event
		callPlayerListenerEvent(&EventListener::onPlayerQueueChange, self, createEvent());
	}

	void Player::onMediaPlaybackProviderPlay(MediaPlaybackProvider* provider) {
		auto self = shared_from_this();
		onMediaPlaybackProviderEvent();
		startPlayerStateInterval();
		// emit state change event
		auto event = createEvent();
		callPlayerListenerEvent(&EventListener::onPlayerStateChange, self, event);
		// emit play event
		callPlayerListenerEvent(&EventListener::onPlayerPlay, self, event);
	}

	void Player::onMediaPlaybackProviderPause(MediaPlaybackProvider* provider) {
		auto self = shared_from_this();
		onMediaPlaybackProviderEvent();
		stopPlayerStateInterval();
		// emit state change event
		auto event = createEvent();
		callPlayerListenerEvent(&EventListener::onPlayerStateChange, self, event);
		// emit pause event
		callPlayerListenerEvent(&EventListener::onPlayerPause, self, event);
	}

	void Player::onMediaPlaybackProviderTrackFinish(MediaPlaybackProvider* provider) {
		auto self = shared_from_this();
		onMediaPlaybackProviderEvent();
		stopPlayerStateInterval();
		saveInBackground({.includeMetadata=false});
		
		// emit track finish event
		callPlayerListenerEvent(&EventListener::onPlayerTrackFinish, self, createEvent());
		
		organizer->next().toVoid().except([=](std::exception_ptr error) {
			printf("Unable to continue to next track: %s\n", utils::getExceptionDetails(error).c_str());
		});
	}

	void Player::onMediaPlaybackProviderMetadataChange(MediaPlaybackProvider* provider) {
		auto self = shared_from_this();
		onMediaPlaybackProviderEvent();
		
		// emit metadata change event
		callPlayerListenerEvent(&EventListener::onPlayerMetadataChange, self, createEvent());
	}

	void Player::onMediaPlaybackProviderEvent() {
		auto playbackProvider = this->mediaProvider->player();
		auto providerState = (playbackProvider != nullptr) ? maybe(playbackProvider->state()) : std::nullopt;
		auto providerMetadata = (playbackProvider != nullptr) ? maybe(playbackProvider->metadata()) : std::nullopt;
		auto providerTrack = (providerMetadata) ? providerMetadata->currentTrack : nullptr;
		auto prevPlayingTrack = this->playingTrack;
		auto organizerTrack = organizer->getCurrentTrack();
		
		this->providerPlaybackState = providerState;
		this->providerPlaybackMetadata = providerMetadata;
		
		if(organizerTrack != nullptr && providerTrack != nullptr && organizerTrack->uri() == providerTrack->uri()) {
			this->playingTrack = organizerTrack;
		} else {
			this->playingTrack = providerTrack;
		}
		
		/*if(this->playingTrack != nullptr && this->playingTrack != prevPlayingTrack) {
			// TODO update now playing
		}
		if(providerState) {
			// TODO update nowplaying position
		}*/
	}

	Player::Event Player::createEvent() {
		return Event{
			.metadata=this->metadata(),
			.state=this->state(),
			.context=this->organizer->getContext(),
			.queue=this->organizer->getQueue()
		};
	}
}

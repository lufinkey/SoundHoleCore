//
//  Player.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 2/9/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "Player.hpp"
#include <soundhole/utils/Utils.hpp>
#include "Player_objc_private.hpp"

namespace sh {
	$<Player> Player::new$(Options options) {
		return fgl::new$<Player>(options);
	}

	Player::Player(Options options)
	: options(options),
	organizer(PlaybackOrganizer::new$({.delegate=this})),
	mediaProvider(nullptr),
	preparedMediaProvider(nullptr),
	systemMediaControls(nullptr) {
		organizer->addEventListener(this);
	}

	Player::~Player() {
		if(systemMediaControls != nullptr) {
			systemMediaControls->removeListener(this);
		}
		organizer->removeEventListener(this);
		organizer->stop();
		setMediaProvider(nullptr);
		stopPlayerStateInterval();
		#if defined(TARGETPLATFORM_IOS)
		deleteObjcListenerWrappers();
		#endif
	}



	#pragma mark Saving / Loading

	String Player::getProgressFilePath() const {
		return options.savePrefix+"/player_progress.json";
	}

	String Player::getMetadataFilePath() const {
		return options.savePrefix+"/player_metadata.json";
	}

	Promise<void> Player::load(MediaProviderStash* stash) {
		if(options.savePrefix.empty()) {
			return Promise<void>::resolve();
		}
		auto runOptions = AsyncQueue::RunOptions{
			.tag="load"
		};
		return saveQueue.run(runOptions, [=](auto task) {
			return async<Optional<ProgressData>>([=]() -> Optional<ProgressData> {
				// load progress data
				auto progressPath = getProgressFilePath();
				if(fs::exists(progressPath)) {
					auto data = fs::readFile(progressPath);
					std::string jsonError;
					auto json = Json::parse(data, jsonError);
					if(!json.is_null()) {
						return ProgressData::fromJson(json);
					}
				}
				return std::nullopt;
			}).then([=](Optional<ProgressData> progressData) {
				// load organizer
				auto metadataPath = getMetadataFilePath();
				return organizer->load(metadataPath, stash).then([=](bool loaded) {
					// apply progress data
					this->resumableProgress = progressData;
				});
			});
		}).promise;
	}

	Promise<void> Player::save(SaveOptions options) {
		auto runOptions = AsyncQueue::RunOptions{
			.tag = options.includeMetadata ? "metadata" : "progress"
		};
		return saveQueue.run(runOptions, [=](auto task) {
			return performSave(options);
		}).promise;
	}

	void Player::saveInBackground(SaveOptions options) {
		auto runOptions = options.includeMetadata ?
			AsyncQueue::RunOptions{
				.tag = "bgMetadata",
				.cancelTags = { "bgMetadata", "bgProgress" }
			} : AsyncQueue::RunOptions{
				.tag = "bgProgress",
				.cancelTags = { "bgProgress" }
			};
		saveQueue.run(runOptions, [=](auto task) {
			return performSave(options);
		});
	}

	Promise<void> Player::performSave(SaveOptions options) {
		auto currentTrack = organizer->getCurrentTrack();
		auto playbackState = providerPlaybackState;
		auto metadataPromise = Promise<void>::resolve();
		if(options.includeMetadata) {
			auto metadataPath = getMetadataFilePath();
			metadataPromise = organizer->save(metadataPath);
		}
		return metadataPromise.then([=]() {
			auto progressPath = getProgressFilePath();
			if(currentTrack && playbackState) {
				auto progressData = ProgressData{
					.uri = currentTrack->uri(),
					.providerName = currentTrack->mediaProvider()->name(),
					.position = playbackState->position
				};
				return async<void>([=]() {
					auto json = progressData.toJson().dump();
					fs::writeFile(progressPath, json);
				});
			} else {
				return async<void>([=]() {
					if(fs::exists(progressPath)) {
						fs::removeAll(progressPath);
					}
				});
			}
		});
	}



	#pragma mark Event Listeners

	void Player::addEventListener(EventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		listeners.pushBack(listener);
	}

	void Player::removeEventListener(EventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		listeners.removeLastEqual(listener);
	}

	bool Player::hasEventListener(EventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		return listeners.contains(listener);
	}



	#pragma mark Media Control functions

	Promise<void> Player::play($<Track> track) {
		#ifdef TARGETPLATFORM_IOS
		if(track) {
			activateAudioSession();
		}
		#endif
		return organizer->play(track);
	}

	Promise<void> Player::play($<TrackCollectionItem> item) {
		#ifdef TARGETPLATFORM_IOS
		if(item) {
			activateAudioSession();
		}
		#endif
		return organizer->play(item);
	}

	Promise<void> Player::play($<QueueItem> queueItem) {
		#ifdef TARGETPLATFORM_IOS
		if(queueItem) {
			activateAudioSession();
		}
		#endif
		return organizer->play(queueItem);
	}

	Promise<void> Player::play(ItemVariant item) {
		#ifdef TARGETPLATFORM_IOS
			if(std::get_if<NoItem>(&item) == nullptr) {
				activateAudioSession();
			}
		#endif
		return organizer->play(item);
	}

	Promise<void> Player::playAtQueueIndex(size_t index) {
		auto item = organizer->getQueueItem(index);
		#ifdef TARGETPLATFORM_IOS
		if(item) {
			activateAudioSession();
		}
		#endif
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
				console::error("Error while player was auto-starting from queue: ", utils::getExceptionDetails(error).fullDescription);
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



	#pragma mark Metadata / State / Context / Queue

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

	Optional<size_t> Player::previousContextIndex() const {
		return organizer->getPreviousContextIndex();
	}

	Optional<size_t> Player::nextContextIndex() const {
		return organizer->getNextContextIndex();
	}

	LinkedList<$<QueueItem>> Player::queueItems() const {
		return organizer->getQueue();
	}



	#pragma mark SystemMediaControls

	void Player::setSystemMediaControls(SystemMediaControls* controls) {
		if(systemMediaControls != nullptr) {
			systemMediaControls->removeListener(this);
		}
		systemMediaControls = controls;
		if(systemMediaControls != nullptr) {
			systemMediaControls->addListener(this);
			auto metadata = this->metadata();
			if(metadata.currentTrack) {
				updateSystemMediaControls();
			}
		}
	}

	SystemMediaControls* Player::getSystemMediaControls() {
		return systemMediaControls;
	}

	const SystemMediaControls* Player::getSystemMediaControls() const {
		return systemMediaControls;
	}

	void Player::updateSystemMediaControls() {
		using PlaybackState = SystemMediaControls::PlaybackState;
		using ButtonState = SystemMediaControls::ButtonState;
		using RepeatMode = SystemMediaControls::RepeatMode;
		if(systemMediaControls != nullptr) {
			auto context = this->context();
			auto contextIndex = this->contextIndex();
			auto state = this->state();
			auto metadata = this->metadata();
			
			auto playbackState = metadata.currentTrack ? (state.playing ? PlaybackState::PLAYING : PlaybackState::PAUSED) : PlaybackState::STOPPED;
			systemMediaControls->setPlaybackState(playbackState);
			
			systemMediaControls->setButtonState(ButtonState{
				.playEnabled = (metadata.currentTrack != nullptr),
				.pauseEnabled = (metadata.currentTrack != nullptr),
				.stopEnabled = (metadata.currentTrack != nullptr),
				.previousEnabled = (metadata.previousTrack != nullptr),
				.nextEnabled = (metadata.nextTrack != nullptr),
				.repeatEnabled = false,
				.repeatMode = RepeatMode::OFF,
				.shuffleEnabled = true,
				.shuffleMode = state.shuffling
			});
			
			auto nowPlaying = SystemMediaControls::NowPlaying();
			if(metadata.currentTrack) {
				nowPlaying.title = metadata.currentTrack->name();
				auto artists = metadata.currentTrack->artists();
				if(artists.size() > 0) {
					nowPlaying.artist = String::join(metadata.currentTrack->artists().map<String>([](auto& a){ return a->name(); }), ", ");
				}
				if(!metadata.currentTrack->albumURI().empty()) {
					nowPlaying.albumTitle = metadata.currentTrack->albumName();
				}
				if(auto duration = metadata.currentTrack->duration()) {
					nowPlaying.duration = duration;
				}
				nowPlaying.elapsedTime = state.position;
			}
			if(contextIndex) {
				nowPlaying.trackIndex = contextIndex;
			}
			if(context) {
				auto itemCount = context->itemCount();
				if(itemCount) {
					nowPlaying.trackCount = itemCount;
				}
			}
			systemMediaControls->setNowPlaying(nowPlaying);
		}
	}



	#pragma mark Preparing / Playing Track

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
				preparedPlaybackProvider->stop();
				preparedMediaProvider = nullptr;
			}
			auto currentPlaybackProvider = (self->mediaProvider != nullptr) ? self->mediaProvider->player() : nullptr;
			if(currentPlaybackProvider != nullptr && currentPlaybackProvider != playbackProvider) {
				currentPlaybackProvider->stop();
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
				updateSystemMediaControls();
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
		if(this->mediaProvider != nullptr && this->mediaProvider == this->preparedMediaProvider) {
			this->preparedMediaProvider = nullptr;
		}
		if (playbackProvider != nullptr) {
			playbackProvider->addEventListener(this);
			if(playbackProvider->state().playing) {
				startPlayerStateInterval();
			}
		}
	}



	#pragma mark Player State Interval Timer
	
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
				console::error("Error preparing next track: ", utils::getExceptionDetails(error).fullDescription);
			});
		}
		// save player state if needed
		auto currentTime = std::chrono::steady_clock::now();
		if(!lastSaveTime || (currentTime - lastSaveTime.value()) >= std::chrono::milliseconds((size_t)(options.progressSaveInterval * 1000))) {
			lastSaveTime = currentTime;
			saveInBackground({.includeMetadata=false});
		}
	}



	#pragma mark PlaybackOrganizer::EventListener

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
				// update media controls
				self->updateSystemMediaControls();
				// emit metadata change event
				callPlayerListenerEvent(&EventListener::onPlayerMetadataChange, self, createEvent());
			}).except([=](std::exception_ptr error) {
				console::error("Unable to load previous track from organizer: ", utils::getExceptionDetails(error).fullDescription);
			});
		}
		if(!nextTrackPromise.isComplete()) {
			w$<Player> weakSelf = shared_from_this();
			nextTrackPromise.then([=]($<Track> track) {
				auto self = weakSelf.lock();
				if(!self) {
					return;
				}
				// update media controls
				self->updateSystemMediaControls();
				// emit metadata change event
				callPlayerListenerEvent(&EventListener::onPlayerMetadataChange, self, createEvent());
			}).except([=](std::exception_ptr error) {
				console::error("Unable to load next track from organizer: ", utils::getExceptionDetails(error).fullDescription);
			});
		}
	}

	void Player::onPlaybackOrganizerQueueChange($<PlaybackOrganizer> organizer) {
		auto self = shared_from_this();
		// update media controls
		updateSystemMediaControls();
		// emit queue change event
		callPlayerListenerEvent(&EventListener::onPlayerQueueChange, self, createEvent());
	}



	#pragma mark MediaPlaybackProvider::EventListener

	void Player::onMediaPlaybackProviderPlay(MediaPlaybackProvider* provider) {
		auto self = shared_from_this();
		onMediaPlaybackProviderEvent();
		startPlayerStateInterval();
		#if defined(TARGETPLATFORM_IOS)
			activateAudioSession();
		#endif
		
		// update media controls
		updateSystemMediaControls();
		
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
		
		// update media controls
		updateSystemMediaControls();
		
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
		
		// update media controls
		updateSystemMediaControls();
		
		// emit track finish event
		callPlayerListenerEvent(&EventListener::onPlayerTrackFinish, self, createEvent());
		
		w$<Player> weakSelf = self;
		organizer->next().then([=](bool hasNext) {
			#if defined(TARGETPLATFORM_IOS)
				auto self = weakSelf.lock();
				if(self && !hasNext && !self->state().playing) {
					self->deactivateAudioSession();
				}
			#endif
		}).except([=](std::exception_ptr error) {
			console::error("Unable to continue to next track: ", utils::getExceptionDetails(error).fullDescription);
		});
	}

	void Player::onMediaPlaybackProviderMetadataChange(MediaPlaybackProvider* provider) {
		auto self = shared_from_this();
		onMediaPlaybackProviderEvent();
		
		// update media controls
		updateSystemMediaControls();
		
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



	#pragma mark SystemMediaControls::Listener

	SystemMediaControls::HandlerStatus Player::onSystemMediaControlsPause() {
		using HandlerStatus = SystemMediaControls::HandlerStatus;
		if(metadata().currentTrack) {
			setPlaying(false);
			return HandlerStatus::SUCCESS;
		} else {
			return HandlerStatus::NO_NOW_PLAYING_ITEM;
		}
	}

	SystemMediaControls::HandlerStatus Player::onSystemMediaControlsPlay() {
		using HandlerStatus = SystemMediaControls::HandlerStatus;
		if(metadata().currentTrack) {
			setPlaying(true);
			return HandlerStatus::SUCCESS;
		} else {
			return HandlerStatus::NO_NOW_PLAYING_ITEM;
		}
	}

	SystemMediaControls::HandlerStatus Player::onSystemMediaControlsStop() {
		using HandlerStatus = SystemMediaControls::HandlerStatus;
		// TODO actually stop player
		setPlaying(false);
		return HandlerStatus::SUCCESS;
	}

	SystemMediaControls::HandlerStatus Player::onSystemMediaControlsPrevious() {
		using HandlerStatus = SystemMediaControls::HandlerStatus;
		skipToPrevious();
		return HandlerStatus::SUCCESS;
	}

	SystemMediaControls::HandlerStatus Player::onSystemMediaControlsNext() {
		using HandlerStatus = SystemMediaControls::HandlerStatus;
		skipToNext();
		return HandlerStatus::SUCCESS;
	}

	SystemMediaControls::HandlerStatus Player::onSystemMediaControlsChangeRepeatMode(SystemMediaControls::RepeatMode repeatMode) {
		using HandlerStatus = SystemMediaControls::HandlerStatus;
		if(repeatMode == SystemMediaControls::RepeatMode::ALL) {
			// TODO set repeating
		} else {
			// TODO set not repeating
		}
		return HandlerStatus::FAILED;
	}

	SystemMediaControls::HandlerStatus Player::onSystemMediaControlsChangeShuffleMode(bool shuffleMode) {
		using HandlerStatus = SystemMediaControls::HandlerStatus;
		setShuffling(shuffleMode);
		return HandlerStatus::SUCCESS;
	}



	#pragma mark Helper structs

	Json Player::ProgressData::toJson() const {
		return Json::object{
			{ "uri", (std::string)uri },
			{ "providerName", (std::string)providerName },
			{ "position", position }
		};
	}

	Player::ProgressData Player::ProgressData::fromJson(Json json) {
		return ProgressData{
			.uri = json["uri"].string_value(),
			.providerName = json["providerName"].string_value(),
			.position = json["position"].number_value()
		};
	}
}

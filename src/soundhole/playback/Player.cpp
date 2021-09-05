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
#include "PlayerHistoryManager.hpp"

namespace sh {
	$<Player> Player::new$(MediaDatabase* database, Options options) {
		return fgl::new$<Player>(database, options);
	}

	Player::Player(MediaDatabase* database, Options options)
	: database(database),
	options(options),
	organizer(PlaybackOrganizer::new$(this)),
	mediaProvider(nullptr),
	preparedMediaProvider(nullptr),
	historyManager(nullptr) {
		organizer->addEventListener(this);
		if(options.mediaControls != nullptr) {
			options.mediaControls->addListener(this);
		}
		historyManager = new PlayerHistoryManager(database);
		historyManager->setPlayer(this);
	}

	Player::~Player() {
		if(historyManager != nullptr) {
			delete historyManager;
			historyManager = nullptr;
		}
		if(options.mediaControls != nullptr) {
			options.mediaControls->removeListener(this);
		}
		organizer->removeEventListener(this);
		organizer->stop();
		setMediaProvider(nullptr);
		stopPlayerStateInterval();
		#if defined(TARGETPLATFORM_IOS)
		deleteObjcListenerWrappers();
		#endif
	}

	void Player::setPreferences(Preferences prefs) {
		this->prefs = prefs;
	}

	const Player::Preferences& Player::getPreferences() const {
		return this->prefs;
	}

	void Player::setOrganizerPreferences(PlaybackOrganizer::Preferences organizerPrefs) {
		organizer->setPreferences(organizerPrefs);
	}

	const PlaybackOrganizer::Preferences& Player::getOrganizerPreferences() const {
		return organizer->getPreferences();
	}



	#pragma mark Saving / Loading

	String Player::getProgressFilePath() const {
		return options.savePath+"/player_progress.json";
	}

	String Player::getMetadataFilePath() const {
		return options.savePath+"/player_metadata.json";
	}

	Promise<void> Player::load(MediaProviderStash* stash) {
		if(options.savePath.empty()) {
			return Promise<void>::resolve();
		}
		auto runOptions = AsyncQueue::RunOptions{
			.tag="load"
		};
		return saveQueue.run(runOptions, [=](auto task) {
			return promiseThread([=]() -> Optional<ProgressData> {
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
					// TODO possibly update media controls?
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
				return promiseThread([=]() {
					auto json = progressData.toJson().dump();
					fs::writeFile(progressPath, json);
				});
			} else {
				return promiseThread([=]() {
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
		FGL_ASSERT(track != nullptr, "track cannot be null");
		#ifdef TARGETPLATFORM_IOS
			activateAudioSession();
		#endif
		return organizer->play(track);
	}

	Promise<void> Player::play($<TrackCollectionItem> item) {
		FGL_ASSERT(item != nullptr, "item cannot be null");
		#ifdef TARGETPLATFORM_IOS
			activateAudioSession();
		#endif
		return organizer->play(item);
	}

	Promise<void> Player::play($<QueueItem> queueItem) {
		FGL_ASSERT(queueItem != nullptr, "queueItem cannot be null");
		#ifdef TARGETPLATFORM_IOS
			activateAudioSession();
		#endif
		return organizer->play(queueItem);
	}

	Promise<void> Player::play(PlayerItem item) {
		FGL_ASSERT(item.track() != nullptr, "item track cannot be null");
		#ifdef TARGETPLATFORM_IOS
			activateAudioSession();
		#endif
		return organizer->play(item);
	}

	Promise<void> Player::playAtQueueIndex(size_t index) {
		auto item = organizer->getQueueItem(index);
		#ifdef TARGETPLATFORM_IOS
			activateAudioSession();
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
		auto queueItem = organizer->addToQueue(track);
		saveInBackground({.includeMetadata=true});
		return queueItem;
	}

	$<QueueItem> Player::addToQueueFront($<Track> track) {
		auto queueItem = organizer->addToQueueFront(track);
		saveInBackground({.includeMetadata=true});
		return queueItem;
	}

	$<QueueItem> Player::addToQueueRandomly($<Track> track) {
		auto queueItem = organizer->addToQueueRandomly(track);
		saveInBackground({.includeMetadata=true});
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
			auto currentTrack = currentItem ? currentItem->track() : nullptr;
			if(currentTrack && resumableProgress->uri == currentTrack->uri() && resumableProgress->providerName == currentTrack->mediaProvider()->name()) {
				return organizer->play(currentItem.value());
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
		auto currentItem = this->currentItem();
		auto currentTrack = currentItem ? currentItem->track() : nullptr;
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
	
	Optional<PlayerItem> Player::currentItem() const {
		auto organizerItem = organizer->getCurrentItem();
		auto organizerTrack = organizerItem ? organizerItem->track() : nullptr;
		if(playingItem.hasValue()) {
			auto playingTrack = playingItem->track();
			// check if playing track doesn't match organizer track
			if(playingTrack == nullptr || !organizerTrack || playingTrack->uri() != organizerTrack->uri() || playingItem->type() != organizerItem->type()) {
				// since playing track is different than organizer track, return playing item
				return playingItem;
			}
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

	ArrayList<$<QueueItem>> Player::queuePastItems() const {
		return organizer->getQueuePastItems();
	}

	ArrayList<$<QueueItem>> Player::queueItems() const {
		return organizer->getQueueItems();
	}



	#pragma mark SystemMediaControls

	void Player::setMediaControls(MediaControls* controls) {
		if(options.mediaControls != nullptr) {
			options.mediaControls->removeListener(this);
		}
		options.mediaControls = controls;
		if(options.mediaControls != nullptr) {
			options.mediaControls->addListener(this);
			auto metadata = this->metadata();
			if(metadata.currentTrack) {
				updateMediaControls();
			}
		}
	}

	MediaControls* Player::getMediaControls() {
		return options.mediaControls;
	}

	const MediaControls* Player::getMediaControls() const {
		return options.mediaControls;
	}

	void Player::updateMediaControls() {
		using PlaybackState = MediaControls::PlaybackState;
		using ButtonState = MediaControls::ButtonState;
		using RepeatMode = MediaControls::RepeatMode;
		if(options.mediaControls != nullptr) {
			auto context = this->context();
			auto contextIndex = this->contextIndex();
			auto state = this->state();
			auto metadata = this->metadata();
			
			auto playbackState = metadata.currentTrack ? (state.playing ? PlaybackState::PLAYING : PlaybackState::PAUSED) : PlaybackState::STOPPED;
			options.mediaControls->setPlaybackState(playbackState);
			
			options.mediaControls->setButtonState(ButtonState{
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
			
			auto nowPlaying = MediaControls::NowPlaying();
			if(metadata.currentTrack) {
				nowPlaying.title = metadata.currentTrack->name();
				auto artists = metadata.currentTrack->artists();
				if(artists.size() > 0) {
					nowPlaying.artist = String::join(metadata.currentTrack->artists().map([](auto& a){ return a->name(); }), ", ");
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
			options.mediaControls->setNowPlaying(nowPlaying);
		}
	}



	#pragma mark Preparing / Playing Track

	Promise<void> Player::prepareItem(PlayerItem item) {
		auto runOptions = AsyncQueue::RunOptions{
			.tag="prepare",
			.cancelMatchingTags=true
		};
		w$<Player> weakSelf = shared_from_this();
		return playQueue.run(runOptions, [=](auto task) -> Promise<void> {
			auto self = weakSelf.lock();
			if(!self) {
				return resolveVoid();
			}
			auto track = item.track();
			if(!track) {
				// TODO maybe some items will need to load their track asynchronously?
				return resolveVoid();
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

	Promise<void> Player::playItem(PlayerItem item) {
		auto runOptions = AsyncQueue::RunOptions{
			.tag="play",
			.cancelMatchingTags=true
		};
		w$<Player> weakSelf = shared_from_this();
		return playQueue.run(runOptions, [=](auto task) -> Promise<void> {
			auto self = weakSelf.lock();
			if(!self) {
				return resolveVoid();
			}
			auto track = item.track();
			// TODO maybe some items will need to load their tracks asynchronously?
			FGL_ASSERT(track != nullptr, "item cannot have a null track");
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
				updateMediaControls();
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
			providerPlayerStateTimer = Timer::withInterval(std::chrono::milliseconds(100), defaultPromiseQueue(), [=]() {
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
		
		// TODO maybe compare state with previous state? in case of buffering track?
		// emit state change event
		callPlayerListenerEvent(&EventListener::onPlayerStateChange, self, createEvent());
		
		// check if next track needs preparing
		auto metadata = self->metadata();
		if(metadata.currentTrack && playbackState.playing
		   && (playbackState.position + prefs.nextTrackPreloadTime) >= metadata.currentTrack->duration().value_or(0.0)
		   && !organizer->isPreparing() && !organizer->hasPreparedNext()) {
			// prepare next track
			organizer->prepareNextIfNeeded().except([=](std::exception_ptr error) {
				// error
				console::error("Error preparing next track: ", utils::getExceptionDetails(error).fullDescription);
			});
		}
		// save player state if needed
		auto currentTime = std::chrono::steady_clock::now();
		if(!lastSaveTime || (currentTime - lastSaveTime.value()) >= std::chrono::milliseconds((size_t)(prefs.progressSaveInterval * 1000))) {
			lastSaveTime = currentTime;
			saveInBackground({.includeMetadata=false});
		}
	}



	#pragma mark PlaybackOrganizer::EventListener

	Promise<void> Player::onPlaybackOrganizerPrepareItem($<PlaybackOrganizer> organizer, PlayerItem item) {
		return prepareItem(item);
	}

	Promise<void> Player::onPlaybackOrganizerPlayItem($<PlaybackOrganizer> organizer, PlayerItem item) {
		return playItem(item);
	}

	void Player::onPlaybackOrganizerItemChange($<PlaybackOrganizer> organizer) {
		auto self = shared_from_this();
		
		auto currentItem = organizer->getCurrentItem();
		auto prevTrackPromise = organizer->getPreviousTrack();
		auto nextTrackPromise = organizer->getNextTrack();
		
		// emit organizer item change event
		if(currentItem) {
			// emit item change event
			callPlayerListenerEvent(&EventListener::onPlayerOrganizerItemChange, self, currentItem.value());
		}
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
				self->updateMediaControls();
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
				self->updateMediaControls();
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
		updateMediaControls();
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
		updateMediaControls();
		
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
		updateMediaControls();
		
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
		updateMediaControls();
		
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
		updateMediaControls();
		
		// emit metadata change event
		callPlayerListenerEvent(&EventListener::onPlayerMetadataChange, self, createEvent());
	}

	void Player::onMediaPlaybackProviderEvent() {
		auto playbackProvider = this->mediaProvider->player();
		auto providerState = (playbackProvider != nullptr) ? maybe(playbackProvider->state()) : std::nullopt;
		auto providerMetadata = (playbackProvider != nullptr) ? maybe(playbackProvider->metadata()) : std::nullopt;
		auto providerTrack = (providerMetadata) ? providerMetadata->currentTrack : nullptr;
		auto prevPlayingItem = this->playingItem;
		auto prevPlayingTrack = prevPlayingItem ? prevPlayingItem->track() : nullptr;
		auto organizerItem = organizer->getCurrentItem();
		auto organizerTrack = organizerItem ? organizerItem->track() : nullptr;
		
		this->providerPlaybackState = providerState;
		this->providerPlaybackMetadata = providerMetadata;
		
		if(organizerTrack != nullptr && providerTrack != nullptr && organizerTrack->uri() == providerTrack->uri()) {
			this->playingItem = organizerItem;
		} else if(prevPlayingTrack != nullptr && providerTrack != nullptr && prevPlayingTrack->uri() == providerTrack->uri()) {
			this->playingItem = prevPlayingItem;
		} else {
			if(providerTrack == nullptr) {
				if(organizerItem.hasValue()) {
					this->playingItem = organizerItem;
				} else {
					this->playingItem = std::nullopt;
				}
			} else {
				this->playingItem = providerTrack;
			}
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
			.metadata = this->metadata(),
			.state = this->state(),
			.context = this->organizer->getContext()
		};
	}



	#pragma mark SystemMediaControls::Listener

	MediaControls::HandlerStatus Player::onMediaControlsPause() {
		using HandlerStatus = MediaControls::HandlerStatus;
		if(metadata().currentTrack) {
			setPlaying(false);
			return HandlerStatus::SUCCESS;
		} else {
			return HandlerStatus::NO_NOW_PLAYING_ITEM;
		}
	}

	MediaControls::HandlerStatus Player::onMediaControlsPlay() {
		using HandlerStatus = MediaControls::HandlerStatus;
		if(metadata().currentTrack) {
			setPlaying(true);
			return HandlerStatus::SUCCESS;
		} else {
			return HandlerStatus::NO_NOW_PLAYING_ITEM;
		}
	}

	MediaControls::HandlerStatus Player::onMediaControlsStop() {
		using HandlerStatus = MediaControls::HandlerStatus;
		// TODO actually stop player
		setPlaying(false);
		return HandlerStatus::SUCCESS;
	}

	MediaControls::HandlerStatus Player::onMediaControlsPrevious() {
		using HandlerStatus = MediaControls::HandlerStatus;
		skipToPrevious();
		return HandlerStatus::SUCCESS;
	}

	MediaControls::HandlerStatus Player::onMediaControlsNext() {
		using HandlerStatus = MediaControls::HandlerStatus;
		skipToNext();
		return HandlerStatus::SUCCESS;
	}

	MediaControls::HandlerStatus Player::onMediaControlsChangeRepeatMode(MediaControls::RepeatMode repeatMode) {
		using HandlerStatus = MediaControls::HandlerStatus;
		if(repeatMode == MediaControls::RepeatMode::ALL) {
			// TODO set repeating
		} else {
			// TODO set not repeating
		}
		return HandlerStatus::FAILED;
	}

	MediaControls::HandlerStatus Player::onMediaControlsChangeShuffleMode(bool shuffleMode) {
		using HandlerStatus = MediaControls::HandlerStatus;
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

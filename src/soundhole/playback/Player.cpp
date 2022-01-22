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
#include "ScrobbleManager.hpp"

namespace sh {
	$<Player> Player::new$(MediaDatabase* database, $<StreamPlayer> streamPlayer, Options options) {
		return fgl::new$<Player>(database, streamPlayer, options);
	}

	Player::Player(MediaDatabase* database, $<StreamPlayer> streamPlayer, Options options)
	: database(database),
	options(options),
	organizer(PlaybackOrganizer::new$(this)),
	streamPlaybackProvider(new StreamPlaybackProvider(streamPlayer)),
	playbackProvider(nullptr),
	preparedPlaybackProvider(nullptr),
	historyManager(nullptr),
	scrobbleManager(nullptr) {
		organizer->addEventListener(this);
		if(options.mediaControls != nullptr) {
			options.mediaControls->addListener(this);
		}
		historyManager = new PlayerHistoryManager(database);
		historyManager->setPlayer(this);
		scrobbleManager = new ScrobbleManager(database);
		scrobbleManager->setPlayer(this);
	}

	Player::~Player() {
		scrobbleManager->setPlayer(nullptr);
		historyManager->setPlayer(nullptr);
		delete scrobbleManager;
		delete historyManager;
		if(options.mediaControls != nullptr) {
			options.mediaControls->setNowPlaying(MediaControls::NowPlaying{});
			options.mediaControls->removeListener(this);
		}
		organizer->removeEventListener(this);
		organizer->stop();
		setPlaybackProvider(nullptr);
		stopPlayerStateInterval();
		#if defined(TARGETPLATFORM_IOS)
		deleteObjcListenerWrappers();
		#endif
		if(streamPlaybackProvider != nullptr) {
			delete streamPlaybackProvider;
		}
	}

	void Player::setPreferences(Preferences prefs) {
		this->prefs = prefs;
		// TODO queue background save
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

	void Player::setScrobblePreferences(ScrobblePreferences scrobblePrefs) {
		this->scrobblePrefs = scrobblePrefs;
		// TODO queue background save
	}

	const Player::ScrobblePreferences& Player::getScrobblePreferences() const {
		return this->scrobblePrefs;
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
			return resolveVoid();
		}
		w$<Player> weakSelf = shared_from_this();
		auto runOptions = AsyncQueue::RunOptions{
			.tag="load"
		};
		return saveQueue.run(runOptions, [=](auto task) -> Promise<void> {
			auto self = weakSelf.lock();
			if(!self) {
				return resolveVoid();
			}
			return promiseThread([=]() -> Optional<ProgressData> {
				// load progress data
				auto progressPath = self->getProgressFilePath();
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
				auto metadataPath = self->getMetadataFilePath();
				return self->organizer->load(metadataPath, stash).then([=](bool loaded) {
					// apply progress data
					self->resumableProgress = progressData;
					// TODO possibly update media controls/state?
				});
			});
		}).promise;
	}

	Promise<void> Player::save(SaveOptions options) {
		w$<Player> weakSelf = shared_from_this();
		auto runOptions = AsyncQueue::RunOptions{
			.tag = options.includeMetadata ? "metadata" : "progress"
		};
		return saveQueue.run(runOptions, [=](auto task) -> Promise<void> {
			auto self = weakSelf.lock();
			if(!self) {
				return resolveVoid();
			}
			return self->performSave(options);
		}).promise;
	}

	void Player::saveInBackground(SaveOptions options) {
		w$<Player> weakSelf = shared_from_this();
		auto runOptions = options.includeMetadata ?
			AsyncQueue::RunOptions{
				.tag = "bgMetadata",
				.cancelTags = { "bgMetadata", "bgProgress" }
			} : AsyncQueue::RunOptions{
				.tag = "bgProgress",
				.cancelTags = { "bgProgress" }
			};
		saveQueue.run(runOptions, [=](auto task) -> Promise<void> {
			auto self = weakSelf.lock();
			if(!self) {
				return resolveVoid();
			}
			return self->performSave(options);
		});
	}

	Promise<void> Player::performSave(SaveOptions options) {
		auto self = shared_from_this();
		auto currentTrack = this->currentTrack();
		auto playbackState = this->providerPlaybackState;
		auto metadataPromise = Promise<void>::resolve();
		if(options.includeMetadata) {
			auto metadataPath = getMetadataFilePath();
			metadataPromise = organizer->save(metadataPath);
		}
		return metadataPromise.then([=]() {
			auto progressPath = self->getProgressFilePath();
			if(currentTrack && playbackState) {
				auto progressData = ProgressData{
					.uri = currentTrack->uri(),
					.name = currentTrack->name(),
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
		if(playbackProvider == nullptr) {
			return resolveVoid();
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

	bool Player::removeFromQueue($<QueueItem> queueItem) {
		bool removed = organizer->removeFromQueue(queueItem);
		saveInBackground({.includeMetadata=true});
		return removed;
	}

	bool Player::clearQueue() {
		bool removed = organizer->clearQueue();
		saveInBackground({.includeMetadata=true});
		return removed;
	}



	Promise<void> Player::setPlaying(bool playing) {
		// if there is a track waiting to be resumed from save, resume it
		auto resumableProgress = this->resumableProgress;
		if(resumableProgress && this->playbackProvider == nullptr && playing) {
			auto currentItem = organizer->getCurrentItem();
			auto resumingTrack = currentItem ? currentItem->linkedTrackWhere([&](auto& cmpTrack) {
				return (cmpTrack->uri() == resumableProgress->uri
					&& (!cmpTrack->uri().empty() || cmpTrack->name() == resumableProgress->name)
					&& cmpTrack->mediaProvider()->name() == resumableProgress->providerName);
			}) : nullptr;
			if(resumingTrack) {
				return organizer->play(currentItem.value());
			}
		}
		// if there is no playback provider, return without doing anything
		if(this->playbackProvider == nullptr) {
			return resolveVoid();
		}
		// play from playback provider
		return this->playbackProvider->setPlaying(playing);
	}

	void Player::setShuffling(bool shuffling) {
		this->organizer->setShuffling(shuffling);
		// emit state change event from main thread
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
		auto previousItemPromise = organizer->getPreviousItem();
		$<Track> previousTrack;
		if(previousItemPromise.isComplete()) {
			auto previousItem = previousItemPromise.get();
			previousTrack = previousItem ? previousItem->track() : nullptr;
		}
		auto nextItemPromise = organizer->getNextItem();
		$<Track> nextTrack;
		if(nextItemPromise.isComplete()) {
			auto nextItem = nextItemPromise.get();
			nextTrack = nextItem ? nextItem->track() : nullptr;
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
		if(resumableProgress && !playbackState->playing) {
			auto currentItem = organizer->getCurrentItem();
			auto resumingTrack = currentItem ? currentItem->linkedTrackWhere([&](auto& cmpTrack) {
				return (cmpTrack->uri() == resumableProgress->uri
					&& (!cmpTrack->uri().empty() || cmpTrack->name() == resumableProgress->name)
					&& cmpTrack->mediaProvider()->name() == resumableProgress->providerName);
			}) : nullptr;
			if(resumingTrack != nullptr) {
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
		if(playingItem.hasValue()) {
			if(organizerItem->matches(playingItem.value()) || (playingItem->isTrack() && organizerItem->linksTrack(playingItem->asTrack()))) {
				return organizerItem;
			}
			return playingItem;
		}
		return organizerItem;
	}

	$<Track> Player::currentTrack() const {
		if(playingTrack) {
			return playingTrack;
		}
		auto organizerItem = organizer->getCurrentItem();
		return organizerItem ? organizerItem->track() : nullptr;
	}
	
	$<PlaybackHistoryItem> Player::currentHistoryItem() const {
		return historyManager->currentItem();
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

	$<Track> Player::preferredTrackForItem(const PlayerItem& item) const {
		auto track = item.track();
		if(auto omniTrack = track.as<OmniTrack>(); omniTrack != nullptr && !omniTrack->linkedTracks().empty()) {
			// check each preferred provider for a playable track
			for(auto& providerName : prefs.preferredProviders) {
				auto providerTrack = omniTrack->linkedTrackWhere([&](auto& cmpTrack) {
					return cmpTrack->mediaProvider()->name() == providerName;
				});
				if(providerTrack && providerTrack->isPlayable()) {
					return providerTrack;
				}
				if(omniTrack->mediaProvider()->name() == providerName && omniTrack->isPlayable()) {
					return omniTrack;
				}
			}
			// look for first confirmed playable track
			auto playableTrack = omniTrack->linkedTrackWhere([](auto& cmpTrack) {
				auto playable = cmpTrack->playable();
				return playable.hasValue() && playable.value();
			});
			if(playableTrack) {
				return playableTrack;
			}
			// look for the first potentially playable track
			playableTrack = item.linkedTrackWhere([](auto& cmpTrack) {
				return cmpTrack->isPlayable();
			});
			if(playableTrack) {
				return playableTrack;
			}
			// look for first preferred track, playable or not
			for(auto& providerName : prefs.preferredProviders) {
				auto providerTrack = item.linkedTrackWhere([&](auto& cmpTrack) {
					return cmpTrack->mediaProvider()->name() == providerName;
				});
				if(providerTrack) {
					return providerTrack;
				}
			}
			// get first track
			return omniTrack->linkedTracks().front();
		}
		return track;
	}

	Generator<void> Player::preparePreferredTrack(PlayerItem item) {
		co_yield setGenResumeQueue(DispatchQueue::main());
		co_yield initialGenNext();
		auto track = item.track();
		if(!track) {
			// TODO maybe some items will need to load their track asynchronously?
			co_return;
		}
		// get full track data
		auto mainTrackDataPromise = track->fetchDataIfNeeded();
		// handle omni-track
		if(auto omniTrack = track.as<OmniTrack>(); omniTrack != nullptr && !omniTrack->linkedTracks().empty()) {
			// check if a preferred playable track is available
			auto preferredTrack = preferredTrackForItem(item);
			if(preferredTrack && preferredTrack->isPlayable() && preferredTrack->playable().hasValue()
			   && prefs.preferredProviders.containsWhere([&](auto& provName) { return preferredTrack->name() == provName; })) {
				co_return;
			}
			// load track data for preferred providers
			auto fetchedTracks = LinkedList<$<Track>>();
			auto trackDataPromises = LinkedList<Promise<void>>();
			for(auto& providerName : prefs.preferredProviders) {
				auto providerTrack = omniTrack->linkedTrackWhere([&](auto& cmpTrack) {
					return cmpTrack->mediaProvider()->name() == providerName;
				});
				// if track URI / provider name is the same as the root track, the root track will apply the fetched data, so we don't need to fetch the data
				if(providerTrack && (providerTrack->uri() != omniTrack->uri() || providerTrack->mediaProvider()->name() != omniTrack->mediaProvider()->name())) {
					trackDataPromises.pushBack(providerTrack->fetchDataIfNeeded());
					fetchedTracks.pushBack(providerTrack);
				}
			}
			// wait for all track data promises so far, then clear
			co_await mainTrackDataPromise;
			for(auto& promise : trackDataPromises) {
				co_await promise;
				co_yield {};
			}
			trackDataPromises.clear();
			// check if a preferred playable track is available
			preferredTrack = preferredTrackForItem(item);
			if(preferredTrack && preferredTrack->isPlayable()
			   && prefs.preferredProviders.containsWhere([&](auto& provName) { return preferredTrack->name() == provName; })) {
				co_return;
			}
			// couldn't find a preferred or playable track, so load remaining tracks
			// load remaining tracks
			for(auto& linkedTrack : omniTrack->linkedTracks()) {
				auto matchIt = fetchedTracks.findEqual(linkedTrack);
				if(matchIt != fetchedTracks.end()) {
					// we already fetched this track, so remove and continue
					fetchedTracks.erase(matchIt);
					continue;
				}
				trackDataPromises.pushBack(linkedTrack->fetchDataIfNeeded());
			}
			// wait for remaining tracks to finish loading
			for(auto& promise : trackDataPromises) {
				co_await promise;
				co_yield {};
			}
		} else {
			co_await mainTrackDataPromise;
		}
	}

	Promise<void> Player::prepareItem(PlayerItem item) {
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "prepare",
			.cancelMatchingTags = true
		};
		w$<Player> weakSelf = shared_from_this();
		return playQueue.run(runOptions, [=](auto task) -> Generator<void> {
			co_yield setGenResumeQueue(DispatchQueue::main());
			co_yield initialGenNext();
			auto self = weakSelf.lock();
			if(!self) {
				co_return;
			}
			{ // load preferred track
				auto gen = preparePreferredTrack(item);
				while(true) {
					auto yieldResult = co_await gen.next();
					co_yield {};
					if(yieldResult.done) {
						break;
					}
				}
			}
			auto track = preferredTrackForItem(item);
			// get track playback provider
			auto provider = track->mediaProvider();
			auto playbackProvider = provider->player();
			// if track has no playback provider, fallback to stream provider
			if(playbackProvider == nullptr) {
				playbackProvider = streamPlaybackProvider;
			}
			// check if new prepared provider will be different than current prepared provider
			auto preparedPlaybackProvider = self->preparedPlaybackProvider;
			if(preparedPlaybackProvider != nullptr && preparedPlaybackProvider != playbackProvider) {
				// clear old prepared provider
				self->preparedPlaybackProvider = nullptr;
			}
			// prepare track and update prepared provider
			co_await playbackProvider->prepare(track);
			self->preparedPlaybackProvider = playbackProvider;
		}).promise;
	}

	Promise<void> Player::playItem(PlayerItem item) {
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "play",
			.cancelTags = { "play", "prepare" }
		};
		w$<Player> weakSelf = shared_from_this();
		return playQueue.run(runOptions, [=](auto task) -> Generator<void> {
			co_yield setGenResumeQueue(DispatchQueue::main());
			co_yield initialGenNext();
			auto self = weakSelf.lock();
			if(!self) {
				co_return;
			}
			{ // load preferred track
				auto gen = preparePreferredTrack(item);
				while(true) {
					auto yieldResult = co_await gen.next();
					co_yield {};
					if(yieldResult.done) {
						break;
					}
				}
			}
			auto track = preferredTrackForItem(item);
			FGL_ASSERT(track != nullptr, "item cannot have a null track");
			// get track playback provider
			auto provider = track->mediaProvider();
			auto playbackProvider = provider->player();
			// if track has no playback provider, fall back to stream provider
			if(playbackProvider == nullptr) {
				playbackProvider = streamPlaybackProvider;
			}
			// check if new provider will be different than current prepared provider
			auto preparedPlaybackProvider = self->preparedPlaybackProvider;
			if(preparedPlaybackProvider != nullptr && preparedPlaybackProvider != playbackProvider) {
				// stop and clear prepared provider
				preparedPlaybackProvider->stop();
				self->preparedPlaybackProvider = nullptr;
			}
			// check if new provider will be different than current provider
			auto currentPlaybackProvider = self->playbackProvider;
			if(currentPlaybackProvider != nullptr && currentPlaybackProvider != playbackProvider) {
				// stop current playback provider
				currentPlaybackProvider->stop();
			}
			setPlaybackProvider(playbackProvider);
			double position = 0;
			auto resumableProgress = self->resumableProgress;
			if(resumableProgress) {
				if(resumableProgress->uri == track->uri() && resumableProgress->providerName == track->mediaProvider()->name()) {
					position = resumableProgress->position;
				} else {
					resumableProgress = std::nullopt;
				}
			}
			co_await playbackProvider->play(track, position);
			self->resumableProgress = std::nullopt;
			self->updateMediaControls();
			self->saveInBackground({.includeMetadata=true});
		}).promise;
	}



	void Player::setPlaybackProvider(MediaPlaybackProvider* playbackProvider) {
		if (playbackProvider == this->playbackProvider) {
			return;
		}
		auto oldPlaybackProvider = this->playbackProvider;
		if (oldPlaybackProvider != nullptr) {
			oldPlaybackProvider->removeEventListener(this);
			stopPlayerStateInterval();
		}
		this->playbackProvider = playbackProvider;
		if(this->playbackProvider != nullptr && this->playbackProvider == this->preparedPlaybackProvider) {
			this->preparedPlaybackProvider = nullptr;
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
		
		if(playbackProvider == nullptr) {
			return;
		}
		auto playbackState = playbackProvider->state();
		this->providerPlaybackState = playbackState;
		
		// TODO maybe compare state with previous state? in case of buffering track?
		// TODO maybe have a separate event for the state change vs position change
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
			// TODO ensure there is no save currently happening, so the save queue doesn't get overloaded
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

	void Player::onPlaybackOrganizerStateChange($<PlaybackOrganizer> organizer) {
		// update playingItem
		auto organizerItem = organizer->getCurrentItem();
		if(organizerItem.hasValue() && playingItem.hasValue()) {
			auto organizerCollectionItem = organizerItem->asCollectionItem();
			auto playingCollectionItem = playingItem->asCollectionItem();
			
			if(organizerCollectionItem && playingCollectionItem) {
				auto organizerShuffledItem = organizerCollectionItem.as<ShuffledTrackCollectionItem>();
				auto playingShuffledItem = playingCollectionItem.as<ShuffledTrackCollectionItem>();
				
				auto organizerSourceItem = organizerShuffledItem ? organizerShuffledItem->sourceItem() : organizerCollectionItem;
				auto playingSourceItem = playingShuffledItem ? playingShuffledItem->sourceItem() : playingCollectionItem;
				
				if(organizerSourceItem && playingSourceItem->matchesItem(organizerSourceItem.get())) {
					playingItem = organizerItem;
				}
			}
		}
	}

	void Player::onPlaybackOrganizerItemChange($<PlaybackOrganizer> organizer) {
		auto self = shared_from_this();
		
		auto currentItem = organizer->getCurrentItem();
		auto prevItemPromise = organizer->getPreviousItem();
		auto nextItemPromise = organizer->getNextItem();
		
		// emit organizer item change event
		if(currentItem) {
			// emit item change event
			callPlayerListenerEvent(&EventListener::onPlayerOrganizerItemChange, self, currentItem.value());
		}
		// emit metadata change event
		callPlayerListenerEvent(&EventListener::onPlayerMetadataChange, self, createEvent());
		
		if(!prevItemPromise.isComplete()) {
			w$<Player> weakSelf = shared_from_this();
			prevItemPromise.then([=](auto item) {
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
		if(!nextItemPromise.isComplete()) {
			w$<Player> weakSelf = shared_from_this();
			nextItemPromise.then([=](auto item) {
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
		auto providerState = (playbackProvider != nullptr) ? maybe(playbackProvider->state()) : std::nullopt;
		auto providerMetadata = (playbackProvider != nullptr) ? maybe(playbackProvider->metadata()) : std::nullopt;
		auto providerTrack = (providerMetadata) ? providerMetadata->currentTrack : nullptr;
		auto prevPlayingItem = this->playingItem;
		auto organizerItem = organizer->getCurrentItem();
		
		this->providerPlaybackState = providerState;
		this->providerPlaybackMetadata = providerMetadata;
		
		if(organizerItem && providerTrack && organizerItem->linksTrack(providerTrack)) {
			this->playingItem = organizerItem;
		} else if(prevPlayingItem && providerTrack && prevPlayingItem->linksTrack(providerTrack)) {
			this->playingItem = prevPlayingItem;
		} else {
			if(providerTrack) {
				this->playingItem = providerTrack;
			} else {
				if(organizerItem) {
					this->playingItem = organizerItem;
				} else {
					this->playingItem = std::nullopt;
				}
			}
		}
		auto matchedTrack = (providerTrack && this->playingItem) ? this->playingItem->linkedTrackWhere([&](auto& cmpTrack) {
			return providerTrack->matches(cmpTrack.get());
		}) : nullptr;
		this->playingTrack = matchedTrack ? matchedTrack : providerTrack;
		
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
			{ "name", (std::string)name },
			{ "providerName", (std::string)providerName },
			{ "position", position }
		};
	}

	Player::ProgressData Player::ProgressData::fromJson(Json json) {
		return ProgressData{
			.uri = json["uri"].string_value(),
			.name = json["name"].string_value(),
			.providerName = json["providerName"].string_value(),
			.position = json["position"].number_value()
		};
	}
}

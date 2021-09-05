//
//  PlayerHistoryManager.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/30/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "PlayerHistoryManager.hpp"
#include <soundhole/utils/Utils.hpp>

namespace sh {
	PlayerHistoryManager::PlayerHistoryManager(MediaDatabase* database)
	: player(nullptr),
	database(database),
	wasPlaying(false) {
		//
	}

	void PlayerHistoryManager::setPlayer(Player* player) {
		if(this->player == player) {
			return;
		}
		if(this->player != nullptr) {
			this->player->removeEventListener(this);
		}
		this->player = player;
		if(this->player != nullptr) {
			this->player->addEventListener(this);
		}
	}

	$<PlaybackHistoryItem> PlayerHistoryManager::currentItem() const {
		return historyItem;
	}

	void PlayerHistoryManager::updateFromPlayer($<Player> player, bool finishedItem) {
		auto currentDate = Date::now();
		auto currentSteadyTime = SteadyClock::now();
		auto currentItem = player->currentItem();
		auto currentTrack = currentItem->track();
		auto playerState = player->state();
		auto collectionItem =
			(currentItem && std::get_if<$<TrackCollectionItem>>(&currentItem.value())) ?
				std::get<$<TrackCollectionItem>>(currentItem.value())
				: nullptr;
		auto context = collectionItem ? collectionItem->context().lock() : nullptr;
		auto& playerPrefs = player->getPreferences();
		
		$<PlaybackHistoryItem> updatedItem;
		$<PlaybackHistoryItem> createdItem;
		bool historyItemNeedsDBUpdate = false;
		
		if(!currentItem) {
			// no current track, so clear history item
			historyItem = nullptr;
			restartedHistoryItem = nullptr;
			nonRestartedHistoryItem = nullptr;
		} else {
			// update active history item if matches
			if(historyItem && historyItem->matchesItem(currentItem.value())) {
				// current item hasn't changed
				double trackDuration = currentTrack->duration().valueOr(PlaybackHistoryItem::FALLBACK_DURATION);
				// get current player position
				double playerPosition = playerState.position;
				if(finishedItem) {
					if(currentTrack->duration()) {
						playerPosition = currentTrack->duration().value();
					} else if(position) {
						auto timeDiff = currentSteadyTime - positionTimePoint;
						double timeDiffSecs = ((double)std::chrono::duration_cast<std::chrono::milliseconds>(timeDiff).count() / 1000.0);
						playerPosition = position.value() + timeDiffSecs;
					}
				}
				// calculate increase in listen duration
				double durationIncrease = 0;
				if(wasPlaying || playerState.playing) {
					if(position) {
						if(playerPosition > position.value()) {
							durationIncrease = playerPosition - position.value();
							auto timeDiff = currentSteadyTime - positionTimePoint;
							double timeDiffSecs = ((double)std::chrono::duration_cast<std::chrono::milliseconds>(timeDiff).count() / 1000.0);
							if(durationIncrease > (timeDiffSecs + 0.1)) {
								durationIncrease = timeDiffSecs;
							}
						}
					}
				}
				// increase history item duration
				if(durationIncrease > 0) {
					historyItem->increaseDuration(durationIncrease);
					updatedItem = historyItem;
					historyItemNeedsDBUpdate = true;
				}
				// check for potentially restarted item
				if(restartedHistoryItem) {
					// we have a potential restart of the track that we're watching
					// increase restated item duration
					if(durationIncrease > 0) {
						restartedHistoryItem->increaseDuration(durationIncrease);
					}
					// check preferences to see if we're still checking for restarts
					if(playerPrefs.minDurationRatioForRepeatSongToBeNewHistoryItem) {
						double restartedDurationRatio = restartedHistoryItem->duration().valueOr(0) / trackDuration;
						// see if restarted item has elapsed enough time to become current item
						if(restartedDurationRatio >= playerPrefs.minDurationRatioForRepeatSongToBeNewHistoryItem.value()) {
							// cache current item with old duration
							historyItem->setDuration(nonRestartedHistoryItem->duration());
							updateHistoryDBIfNeeded(historyItem);
							updatedItem = historyItem;
							// update current history item to restarted item
							historyItem = restartedHistoryItem;
							restartedHistoryItem = nullptr;
							nonRestartedHistoryItem = nullptr;
							createdItem = historyItem;
							historyItemNeedsDBUpdate = true;
						}
					} else {
						restartedHistoryItem = nullptr;
						nonRestartedHistoryItem = nullptr;
					}
				}
				else {
					// check if we should track the song as restarted
					double durationRatio = historyItem->duration().valueOr(0) / trackDuration;
					if(playerPrefs.minDurationRatioForRepeatSongToBeNewHistoryItem && position
					   // if position moved backwards
					   && playerPosition < position.value()
					   // and position is less than the remaining time of the song at the repeat min duration ratio
					   && playerPosition <= ((1.0 - playerPrefs.minDurationRatioForRepeatSongToBeNewHistoryItem.value()) * trackDuration)
					   // and the duration is greater than or equal to the repeat min duration ratio
					   && durationRatio >= playerPrefs.minDurationRatioForRepeatSongToBeNewHistoryItem.value()) {
						// track could have restarted, so track potential restart
						restartedHistoryItem = PlaybackHistoryItem::new$({
							.track = currentTrack,
							.startTime = currentDate,
							.contextURI = context ? context->uri() : nullptr,
							.duration = 0,
							.chosenByUser = true // TODO < this needs to be changed!!! check if player is playing from radio context, autoplay, etc
						});
						// copy current history item into the non-restarted item
						nonRestartedHistoryItem = PlaybackHistoryItem::new$(historyItem->toData());
					}
				}
			}
			else {
				// current item has changed
				// start new history item
				historyItem = PlaybackHistoryItem::new$({
					.track = currentTrack,
					.startTime = currentDate,
					.contextURI = context ? context->uri() : nullptr,
					.duration = 0,
					.chosenByUser = true // TODO < this needs to be changed!!! check if player is playing from radio context, autoplay, etc
				});
				restartedHistoryItem = nullptr;
				nonRestartedHistoryItem = nullptr;
				createdItem = historyItem;
				historyItemNeedsDBUpdate = true;
			}
		}
		
		// check if we have a history item
		if(historyItem) {
			// update current history item in DB
			if(historyItemNeedsDBUpdate) {
				updateHistoryDBIfNeeded(historyItem);
			}
			// update state
			position = playerState.position;
			positionTimePoint = currentSteadyTime;
			wasPlaying = playerState.playing;
		} else {
			// clear current state
			position = std::nullopt;
			positionTimePoint = SteadyClock::time_point();
			wasPlaying = false;
		}
		
		// call listener events
		if(updatedItem) {
			player->callPlayerListenerEvent(&Player::EventListener::onPlayerUpdateHistoryItem, player, updatedItem);
		}
		if(createdItem) {
			player->callPlayerListenerEvent(&Player::EventListener::onPlayerCreateHistoryItem, player, createdItem);
		}
	}

	void PlayerHistoryManager::updateHistoryDBIfNeeded($<PlaybackHistoryItem> historyItem) {
		auto& playerPrefs = player->getPreferences();
		if(!playerPrefs.minDurationForHistory || playerPrefs.minDurationForHistory.value() <= historyItem->duration().valueOr(0)) {
			database->cachePlaybackHistoryItems({ historyItem }).except([](std::exception_ptr error) {
				console::error("Error caching history item: ", utils::getExceptionDetails(error).fullDescription);
			});
		}
	}

	void PlayerHistoryManager::onPlayerStateChange($<Player> player, const Player::Event& event) {
		updateFromPlayer(player, false);
	}

	void PlayerHistoryManager::onPlayerMetadataChange($<Player> player, const Player::Event& event) {
		updateFromPlayer(player, false);
	}

	void PlayerHistoryManager::onPlayerTrackFinish($<Player> player, const Player::Event& event) {
		updateFromPlayer(player, true);
	}

	void PlayerHistoryManager::onPlayerPlay($<Player> player, const Player::Event& event) {
		updateFromPlayer(player, false);
	}

	void PlayerHistoryManager::onPlayerPause($<Player> player, const Player::Event& event) {
		updateFromPlayer(player, false);
	}
}

//
//  ScrobbleManager.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/12/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#include "ScrobbleManager.hpp"
#include <soundhole/utils/Utils.hpp>
#include <random>
#include <uuid.h>

namespace sh {
	ScrobbleManager::ScrobbleManager(Player* player, MediaDatabase* database)
	: player(player),
	database(database),
	uuidGenerator(createUUIDGenerator()) {
		if(this->player != nullptr) {
			this->player->addEventListener(this);
		}
	}

	ScrobbleManager::~ScrobbleManager() {
		if(this->player != nullptr) {
			this->player->removeEventListener(this);
		}
	}

	Function<String()> ScrobbleManager::createUUIDGenerator() {
		std::random_device rd;
		auto seed_data = std::array<int, std::mt19937::state_size> {};
		std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
		std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
		std::mt19937 generator(seq);
		auto gen = fgl::new$<uuids::uuid_random_generator>(generator);
		return [=]() -> String {
			auto& generator = *gen;
			return uuids::to_string(generator());
		};
	}

	$<Scrobble> ScrobbleManager::createScrobble(Scrobbler* scrobbler, $<PlaybackHistoryItem> historyItem, $<Album> album) {
		auto track = historyItem->track();
		auto artists = track->artists();
		auto albumArtists = album ? album->artists() : ArrayList<$<Artist>>();
		return Scrobble::new$(Scrobble::Data{
			.localID = uuidGenerator(),
			.scrobbler = scrobbler,
			.startTime = historyItem->startTime(),
			.trackURI = track->uri(),
			.musicBrainzID = track->musicBrainzID(),
			.trackName = track->name(),
			.artistName = artists.empty() ? String() : artists.front()->name(),
			.albumName = album ? album->name() : track->albumName(),
			.albumArtistName = albumArtists.empty() ? String() : albumArtists.front()->name(),
			.duration = track->duration(),
			.trackNumber = track->trackNumber(),
			.chosenByUser = historyItem->chosenByUser(),
			.historyItemStartTime = historyItem->startTime(),
			.uploaded = false,
			.ignoredReason = std::nullopt
		});
	}



	void ScrobbleManager::scrobble($<PlaybackHistoryItem> historyItem) {
		if(this->historyItemScrobbled &&this->historyItem && this->historyItem->matches(historyItem.get())) {
			if(!uploadBatch) {
				uploadPendingScrobbles();
			}
			return;
		}
		auto track = historyItem->track();
		$<Album> album;
		// try to get track album from player
		if(!track->albumURI().empty()) {
			if(player != nullptr) {
				auto playerContext = std::dynamic_pointer_cast<Album>(player->context());
				if(playerContext != nullptr && playerContext->uri() == track->albumURI()) {
					album = playerContext;
				}
			}
		}
		// TODO fetch album for historyItem if unavailable
		auto provider = track->mediaProvider();
		auto scrobblerStash = database->scrobblerStash();
		if(scrobblerStash == nullptr) {
			return;
		}
		auto& scrobblePrefs = player->getScrobblePreferences();
		// add scrobble for each available scrobbler
		LinkedList<$<Scrobble>> scrobblesToCache;
		for(auto scrobbler : scrobblerStash->getScrobblers()) {
			if(!scrobbler->isLoggedIn()) {
				continue;
			}
			auto scrobblerName = scrobbler->name();
			auto prefs = scrobblePrefs.scrobblers.get(scrobblerName, Player::ScrobblerPreferences());
			if(!prefs.enabled) {
				continue;
			}
			auto providerMode = prefs.providerModes.get(provider->name(), Player::ScrobblerProviderMode::ENABLED);
			switch(providerMode) {
				case Player::ScrobblerProviderMode::ENABLED: {
					// add pending scrobble
					auto pendingScrobblesIt = pendingScrobbles.find(scrobblerName);
					if(pendingScrobblesIt == pendingScrobbles.end()) {
						pendingScrobblesIt = std::get<0>(pendingScrobbles.insert(std::make_pair(scrobblerName, LinkedList<$<Scrobble>>())));
					}
					auto scrobble = createScrobble(scrobbler, historyItem, album);
					scrobblesToCache.pushBack(scrobble);
					pendingScrobblesIt->second.pushBack(scrobble);
				} break;
				
				case Player::ScrobblerProviderMode::ENABLED_ELSEWHERE: {
					// add unmatched scrobble
					auto unmatchedScrobblesIt = unmatchedScrobbles.find(scrobblerName);
					if(unmatchedScrobblesIt == unmatchedScrobbles.end()) {
						unmatchedScrobblesIt = std::get<0>(unmatchedScrobbles.insert(std::make_pair(scrobblerName, LinkedList<UnmatchedScrobble>())));
					}
					unmatchedScrobblesIt->second.pushBack(UnmatchedScrobble{
						.startTime = historyItem->startTime(),
						.trackURI = track->uri()
					});
				} break;
				
				case Player::ScrobblerProviderMode::DISABLED:
				break;
			}
		}
		if(this->historyItem && this->historyItem->matches(historyItem.get())) {
			historyItemScrobbled = true;
		}
		// cache new scrobbles in DB
		if(!scrobblesToCache.empty()) {
			database->cacheScrobbles(scrobblesToCache).except([](std::exception_ptr error) {
				console::error("Error caching scrobbles: ", utils::getExceptionDetails(error).fullDescription);
			});
		}
		// upload scrobbles if needed
		if(!uploadBatch) {
			uploadPendingScrobbles();
		}
	}



	ArrayList<$<Scrobble>> ScrobbleManager::getUploadingScrobbles() {
		if(uploadBatch) {
			return uploadBatch->scrobbles;
		}
		return {};
	}

	ArrayList<$<const Scrobble>> ScrobbleManager::getUploadingScrobbles() const {
		if(uploadBatch) {
			return *((const ArrayList<$<const Scrobble>>*)&uploadBatch->scrobbles);
		}
		return {};
	}

	bool ScrobbleManager::isUploadingScrobbles() const {
		return uploadBatch.hasValue();
	}

	Promise<bool> ScrobbleManager::uploadPendingScrobbles() {
		const bool DoneState = true;
		const bool NotDoneState = false;
		if(uploadBatch) {
			return uploadBatch->promise;
		}
		auto pendingScrobblesIt = pendingScrobbles.findWhere([](auto& pair) {
			return !pair.second.empty();
		});
		if(pendingScrobblesIt == pendingScrobbles.end()) {
			return resolveWith(DoneState);
		}
		auto scrobblerStash = database->scrobblerStash();
		auto scrobblerName = pendingScrobblesIt->first;
		auto scrobbler = scrobblerStash->getScrobbler(scrobblerName);
		if(scrobbler == nullptr) {
			pendingScrobbles.erase(scrobblerName);
			// no scrobbler available, so remove pending scrobbles and
			return uploadPendingScrobbles();
		}
		size_t maxScrobblesForUpload = 50;
		auto uploadingScrobbles = ([&]() {
			auto beginIt = pendingScrobblesIt->second.begin();
			auto endIt = beginIt;
			if(pendingScrobblesIt->second.size() <= maxScrobblesForUpload) {
				endIt = pendingScrobblesIt->second.end();
			} else {
				endIt = std::next(endIt, maxScrobblesForUpload);
			}
			return ArrayList<$<Scrobble>>(beginIt, endIt);
		})();
		// upload the pending scrobbles
		auto promise = ([=]() -> Promise<bool> {
			auto responses = co_await scrobbler->scrobble(uploadingScrobbles);
			// apply responses to scrobbles
			for(auto [i, scrobble] : enumerate(uploadingScrobbles)) {
				auto response = responses[i];
				scrobble->applyResponse(response);
			}
			// cache updated scrobbles
			co_await this->database->cacheScrobbles(uploadingScrobbles);
			// remove scrobbles from pending scrobbles
			auto pendingScrobblesIt = this->pendingScrobbles.find(scrobblerName);
			for(auto& scrobble : uploadingScrobbles) {
				pendingScrobblesIt->second.removeFirstEqual(scrobble);
			}
			// check if there are still any pending scrobbles
			if(!this->pendingScrobbles.containsWhere([](auto& pair) {
				return !pair.second.empty();
			})) {
				auto newPendingScrobbles = (co_await this->database->getScrobbles({
					.filters = {
						.uploaded = false
					},
					.range = sql::IndexRange{
						.startIndex = 0,
						.endIndex = maxScrobblesForUpload
					},
					.order = sql::Order::ASC
				})).items;
				if(newPendingScrobbles.empty()) {
					// no pending scrobbles, so we're done here
					co_return DoneState;
				}
				for(auto& scrobble : newPendingScrobbles) {
					auto scrobblerName = scrobble->scrobbler()->name();
					auto pendingScrobblesIt = this->pendingScrobbles.find(scrobblerName);
					if(pendingScrobblesIt == this->pendingScrobbles.end()) {
						this->pendingScrobbles.insert(std::make_pair(scrobblerName, LinkedList<$<Scrobble>>{ scrobble }));
					} else {
						pendingScrobblesIt->second.pushBack(scrobble);
					}
				}
				co_return NotDoneState;
			}
			co_return NotDoneState;
		})();
		this->uploadBatch = UploadBatch{
			.scrobbler = scrobbler->name(),
			.scrobbles = uploadingScrobbles,
			.promise = promise
		};
		// handle promise finishing
		promise.then([=](bool isDone) {
			this->uploadBatch = std::nullopt;
			if(!isDone) {
				uploadPendingScrobbles();
			}
		}, [=](std::exception_ptr error) {
			this->uploadBatch = std::nullopt;
			console::error("Error uploading scrobbles: ", utils::getExceptionDetails(error).fullDescription);
		});
		return promise;
	}


	void ScrobbleManager::updateFromPlayer($<Player> player, bool finishedItem) {
		// update current history item
		auto historyItem = player->currentHistoryItem();
		if(!historyItem || !this->historyItem || !this->historyItem->matches(historyItem.get())) {
			this->historyItemScrobbled = false;
		}
		this->historyItem = historyItem;
		// calculate elapsed point
		double elapsed = historyItem->duration().valueOr(0);
		double elapsedRatio = elapsed / historyItem->track()->duration().valueOr(PlaybackHistoryItem::FALLBACK_DURATION);
		// scrobble if needed
		auto scrobblePrefs = player->getScrobblePreferences();
		if(!historyItemScrobbled &&
		   ((scrobblePrefs.scrobbleAtElapsed && elapsed >= scrobblePrefs.scrobbleAtElapsed.value())
		   || (scrobblePrefs.scrobbleAtElapsedRatio && elapsedRatio >= scrobblePrefs.scrobbleAtElapsedRatio.value())
		   || (!scrobblePrefs.scrobbleAtElapsed && !scrobblePrefs.scrobbleAtElapsedRatio && finishedItem))) {
			scrobble(historyItem);
		}
	}



	#pragma mark Player::EventListener

	void ScrobbleManager::onPlayerStateChange($<Player> player, const Player::Event& event) {
		updateFromPlayer(player, false);
	}

	void ScrobbleManager::onPlayerMetadataChange($<Player> player, const Player::Event& event) {
		updateFromPlayer(player, false);
	}

	void ScrobbleManager::onPlayerTrackFinish($<Player> player, const Player::Event& event) {
		updateFromPlayer(player, true);
	}

	void ScrobbleManager::onPlayerPlay($<Player> player, const Player::Event& event) {
		updateFromPlayer(player, false);
	}

	void ScrobbleManager::onPlayerPause($<Player> player, const Player::Event& event) {
		updateFromPlayer(player, false);
	}
}

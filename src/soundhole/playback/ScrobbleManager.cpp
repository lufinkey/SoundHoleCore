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
	uuidGenerator(createUUIDGenerator()),
	currentHistoryItemScrobbled(false) {
		if(this->player != nullptr) {
			this->player->addEventListener(this);
		}
	}

	ScrobbleManager::~ScrobbleManager() {
		if(this->player != nullptr) {
			this->player->removeEventListener(this);
		}
	}

	bool ScrobbleManager::ScrobblerData::currentlyAbleToUpload() const {
		return !dailyScrobbleLimitExceeded();
	}

	bool ScrobbleManager::ScrobblerData::dailyScrobbleLimitExceeded() const {
		if(!dailyScrobbleLimitExceededDate) {
			return false;
		}
		auto now = Date::now();
		// check that date is in the past (if it's in the future, the system clock probably changed)
		//  and that less than a day has passed since the date/time when the limit was exceeded
		return dailyScrobbleLimitExceededDate.value() <= now && ((now - dailyScrobbleLimitExceededDate.value()) <= std::chrono::days(1));
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
		if(album && track->albumURI() != album->uri()) {
			console::error("Warning: passed non-matching album to ScrobbleManager::createScrobble");
			album = nullptr;
		}
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
		if(this->currentHistoryItemScrobbled && this->currentHistoryItem && this->currentHistoryItem->matches(historyItem.get())) {
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
				if(playerContext && playerContext->uri() == track->albumURI()) {
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
			auto scrobblerDataIt = scrobblersData.find(scrobblerName);
			if(scrobblerDataIt == scrobblersData.end()) {
				scrobblerDataIt = std::get<0>(scrobblersData.insert(std::make_pair(scrobblerName, ScrobblerData())));
			}
			auto providerMode = prefs.providerModes.get(provider->name(), Player::ScrobblerProviderMode::ENABLED);
			switch(providerMode) {
				case Player::ScrobblerProviderMode::ENABLED: {
					// add pending scrobble
					auto scrobble = createScrobble(scrobbler, historyItem, album);
					scrobblesToCache.pushBack(scrobble);
					scrobblerDataIt->second.pendingScrobbles.pushBack(scrobble);
				} break;
				
				case Player::ScrobblerProviderMode::ENABLED_ELSEWHERE: {
					// add unmatched scrobble
					auto unmatchedScrobble = UnmatchedScrobble{
						.startTime = historyItem->startTime(),
						.trackURI = track->uri()
					};
					scrobblerDataIt->second.unmatchedScrobbles.pushBack(unmatchedScrobble);
				} break;
				
				case Player::ScrobblerProviderMode::DISABLED:
				break;
			}
		}
		if(this->currentHistoryItem && this->currentHistoryItem->matches(historyItem.get())) {
			this->currentHistoryItemScrobbled = true;
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

	Promise<ScrobbleUploadResult> ScrobbleManager::uploadPendingScrobbles() {
		if(uploadBatch) {
			return uploadBatch->promise;
		}
		// find scrobbler that has pending scrobbles and is currently able to perform uploads
		auto scrobblerDataIt = scrobblersData.findWhere([](auto& pair) {
			return !pair.second.pendingScrobbles.empty()
				&& pair.second.currentlyAbleToUpload();
		});
		if(scrobblerDataIt == scrobblersData.end()) {
			return resolveWith(ScrobbleUploadResult::DONE);
		}
		auto scrobblerStash = this->database->scrobblerStash();
		auto scrobblerName = scrobblerDataIt->first;
		auto scrobbler = scrobblerStash->getScrobbler(scrobblerName);
		if(scrobbler == nullptr) {
			// no scrobbler available, so remove pending scrobbles and try uploading any remaining scrobbles for other scrobblers
			scrobblersData.erase(scrobblerName);
			return uploadPendingScrobbles();
		}
		size_t maxScrobblesForUpload = 50;
		auto uploadingScrobbles = ([&]() {
			auto& pendingScrobbles = scrobblerDataIt->second.pendingScrobbles;
			auto beginIt = pendingScrobbles.begin();
			auto endIt = pendingScrobbles.end();
			if(pendingScrobbles.size() > maxScrobblesForUpload) {
				endIt = std::next(beginIt, maxScrobblesForUpload);
			}
			return ArrayList<$<Scrobble>>(beginIt, endIt);
		})();
		// upload the pending scrobbles
		auto promise = ([=]() -> Promise<ScrobbleUploadResult> {
			co_await resumeOnQueue(DispatchQueue::main());
			auto responses = co_await scrobbler->scrobble(uploadingScrobbles);
			// apply responses to scrobbles
			auto scrobblerDataIt = this->scrobblersData.find(scrobblerName);
			if(scrobblerDataIt == this->scrobblersData.end()) {
				scrobblerDataIt = std::get<0>(this->scrobblersData.insert(std::make_pair(scrobblerName, ScrobblerData())));
			}
			for(auto [i, scrobble] : enumerate(uploadingScrobbles)) {
				auto response = responses[i];
				scrobble->applyResponse(response);
				if(response.ignored && response.ignored->code == Scrobble::IgnoredReason::Code::DAILY_SCROBBLE_LIMIT_EXCEEDED
				   && !scrobblerDataIt->second.dailyScrobbleLimitExceeded()) {
					scrobblerDataIt->second.dailyScrobbleLimitExceededDate = Date::now();
					// TODO probably set flag to update limit exceeded date in database
				}
			}
			// cache updated scrobbles
			co_await this->database->cacheScrobbles(uploadingScrobbles);
			// remove each uploaded scrobble from the pending scrobbles list
			scrobblerDataIt = this->scrobblersData.find(scrobblerName);
			if(scrobblerDataIt != this->scrobblersData.end()) {
				for(auto& scrobble : uploadingScrobbles) {
					scrobblerDataIt->second.pendingScrobbles.removeFirstEqual(scrobble);
				}
			}
			// search database for any pending scrobbles for each scrobbler
			auto scrobblerStash = this->database->scrobblerStash();
			for(auto scrobbler : scrobblerStash->getScrobblers()) {
				// get scrobbler data
				auto scrobblerName = scrobbler->name();
				auto scrobblerDataIt = this->scrobblersData.find(scrobblerName);
				if(scrobblerDataIt == this->scrobblersData.end()) {
					scrobblerDataIt = std::get<0>(this->scrobblersData.insert(std::make_pair(scrobblerName, ScrobblerData())));
				}
				// if there are already pending scrobbles for this scrobbler, ignore and continue
				if(!scrobblerDataIt->second.pendingScrobbles.empty()) {
					continue;
				}
				// query scrobbles from DB
				auto newPendingScrobbles = (co_await this->database->getScrobbles({
					.filters = {
						.scrobbler = scrobblerName,
						.uploaded = false
					},
					.range = sql::IndexRange{
						.startIndex = 0,
						.endIndex = maxScrobblesForUpload
					},
					.order = sql::Order::ASC
				})).items;
				if(!newPendingScrobbles.empty()) {
					// we have pending scrobbles, so add them to the list of pending scrobbles
					auto scrobblerDataIt = this->scrobblersData.find(scrobblerName);
					if(scrobblerDataIt == this->scrobblersData.end()) {
						scrobblerDataIt = std::get<0>(this->scrobblersData.insert(std::make_pair(scrobblerName, ScrobblerData())));
					}
					scrobblerDataIt->second.pendingScrobbles.pushBackList(newPendingScrobbles);
				}
			}
			co_await resumeOnQueue(DispatchQueue::main());
			// check if there are still any pending scrobbles for scrobblers that are currently able to upload
			bool hasPendingScrobbles = this->scrobblersData.containsWhere([](auto& pair) {
				return !pair.second.pendingScrobbles.empty() && pair.second.currentlyAbleToUpload();
			});
			// if there are no pending scrobbles, uploading is finished for now
			co_return hasPendingScrobbles ? ScrobbleUploadResult::HAS_MORE_UPLOADS : ScrobbleUploadResult::DONE;
		})();
		// apply current upload task
		this->uploadBatch = UploadBatch{
			.scrobbler = scrobbler->name(),
			.scrobbles = uploadingScrobbles,
			.promise = promise
		};
		// handle promise finishing
		promise.then([=](ScrobbleUploadResult uploadResult) {
			this->uploadBatch = std::nullopt;
			if(uploadResult != ScrobbleUploadResult::DONE) {
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
		if(!historyItem || !this->currentHistoryItem || !this->currentHistoryItem->matches(historyItem.get())) {
			this->currentHistoryItemScrobbled = false;
		}
		this->currentHistoryItem = historyItem;
		if(historyItem) {
			// calculate elapsed point
			double elapsed = historyItem->duration().valueOr(0);
			double elapsedRatio = elapsed / historyItem->track()->duration().valueOr(PlaybackHistoryItem::FALLBACK_DURATION);
			// scrobble if needed
			auto scrobblePrefs = player->getScrobblePreferences();
			if(!currentHistoryItemScrobbled &&
			   ((scrobblePrefs.scrobbleAtElapsed && elapsed >= scrobblePrefs.scrobbleAtElapsed.value())
			   || (scrobblePrefs.scrobbleAtElapsedRatio && elapsedRatio >= scrobblePrefs.scrobbleAtElapsedRatio.value())
			   || (!scrobblePrefs.scrobbleAtElapsed && !scrobblePrefs.scrobbleAtElapsedRatio && finishedItem))) {
				scrobble(historyItem);
			}
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

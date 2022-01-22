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
	ScrobbleManager::ScrobbleManager(MediaDatabase* database)
	: player(nullptr),
	database(database),
	uuidGenerator(createUUIDGenerator()),
	currentHistoryItemScrobbled(false) {
		//
	}

	ScrobbleManager::~ScrobbleManager() {
		if(this->player != nullptr) {
			setPlayer(nullptr);
		}
	}

	void ScrobbleManager::setPlayer(Player* player) {
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

	// TODO add function to load any unmatched/unuploaded scrobbles from DB

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



	bool ScrobbleManager::readyToScrobble($<PlaybackHistoryItem> historyItem, bool finishedItem) const {
		auto scrobblePrefs = player->scrobblePrefs;
		double elapsed = historyItem->duration().valueOr(0);
		double elapsedRatio = elapsed / historyItem->track()->duration().valueOr(PlaybackHistoryItem::FALLBACK_DURATION);
		return ((scrobblePrefs.scrobbleAtElapsed && elapsed >= scrobblePrefs.scrobbleAtElapsed.value())
			|| (scrobblePrefs.scrobbleAtElapsedRatio && elapsedRatio >= scrobblePrefs.scrobbleAtElapsedRatio.value())
			|| (!scrobblePrefs.scrobbleAtElapsed && !scrobblePrefs.scrobbleAtElapsedRatio && finishedItem));
	}

	void ScrobbleManager::scrobble($<PlaybackHistoryItem> historyItem) {
		if(this->currentHistoryItemScrobbled && this->currentHistoryItem && this->currentHistoryItem->matches(historyItem.get())) {
			// current history item is already scrobbled, so just upload any pending scrobbles
			if(!uploadBatch) {
				uploadScrobbles();
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
		LinkedList<UnmatchedScrobble> unmatchedScrobblesToCache;
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
					// only add to pendingScrobbles if less than the size of a single upload
					if(scrobblerDataIt->second.pendingScrobbles.size() < scrobbler->maxScrobblesPerRequest()) {
						scrobblerDataIt->second.pendingScrobbles.pushBack(scrobble);
					}
				} break;
				
				case Player::ScrobblerProviderMode::ENABLED_ELSEWHERE: {
					// add unmatched scrobble
					auto unmatchedScrobble = UnmatchedScrobble{
						.scrobbler = scrobbler,
						.historyItem = historyItem
					};
					unmatchedScrobblesToCache.pushBack(unmatchedScrobble);
					// TODO don't add to unmatchedScrobbles if too large
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
		// TODO cache unmatched scrobbles
		// upload scrobbles if needed
		if(!uploadBatch) {
			uploadScrobbles();
		}
	}



	#pragma mark Scrobble Uploads

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

	Promise<ScrobbleBatchResult> ScrobbleManager::uploadScrobbles() {
		if(uploadBatch) {
			return uploadBatch->promise;
		}
		// find scrobbler that has pending scrobbles and is currently able to perform uploads
		auto scrobblerDataIt = scrobblersData.findWhere([](auto& pair) {
			return !pair.second.pendingScrobbles.empty()
				&& pair.second.currentlyAbleToUpload();
		});
		if(scrobblerDataIt == scrobblersData.end()) {
			return resolveWith(ScrobbleBatchResult::DONE);
		}
		auto scrobblerStash = this->database->scrobblerStash();
		auto scrobblerName = scrobblerDataIt->first;
		auto scrobbler = scrobblerStash->getScrobbler(scrobblerName);
		if(scrobbler == nullptr) {
			// no scrobbler available, so remove pending scrobbles and try uploading any remaining scrobbles for other scrobblers
			scrobblersData.erase(scrobblerName);
			return uploadScrobbles();
		}
		size_t maxScrobblesForUpload = scrobbler->maxScrobblesPerRequest();
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
		auto promise = ([=]() -> Promise<ScrobbleBatchResult> {
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
			co_return hasPendingScrobbles ? ScrobbleBatchResult::HAS_MORE : ScrobbleBatchResult::DONE;
		})();
		// apply current upload task
		this->uploadBatch = UploadBatch{
			.scrobbler = scrobbler->name(),
			.scrobbles = uploadingScrobbles,
			.promise = promise
		};
		// handle promise finishing
		promise.then([=](ScrobbleBatchResult uploadResult) {
			this->uploadBatch = std::nullopt;
			if(uploadResult != ScrobbleBatchResult::DONE) {
				uploadScrobbles();
			}
		}, [=](std::exception_ptr error) {
			this->uploadBatch = std::nullopt;
			console::error("Error uploading scrobbles: ", utils::getExceptionDetails(error).fullDescription);
		});
		return promise;
	}



	#pragma mark Scrobble Matches

	ArrayList<UnmatchedScrobble> ScrobbleManager::getMatchingScrobbles() const {
		if(matchBatch) {
			return matchBatch->scrobbles;
		}
		return {};
	}

	bool ScrobbleManager::isMatchingScrobbles() const {
		return matchBatch.hasValue();
	}

	Promise<ScrobbleBatchResult> ScrobbleManager::matchScrobbles() {
		if(matchBatch) {
			return matchBatch->promise;
		}
		// find a scrobbler that has unmatched scrobbles
		auto scrobblerDataIt = scrobblersData.findWhere([](auto& pair) {
			return !pair.second.unmatchedScrobbles.empty();
		});
		if(scrobblerDataIt == scrobblersData.end()) {
			return resolveWith(ScrobbleBatchResult::DONE);
		}
		auto scrobblerStash = this->database->scrobblerStash();
		auto scrobblerName = scrobblerDataIt->first;
		auto scrobbler = scrobblerStash->getScrobbler(scrobblerName);
		if(scrobbler == nullptr) {
			// no scrobbler available, so remove pending scrobbles and try uploading any remaining scrobbles for other scrobblers
			scrobblersData.erase(scrobblerName);
			return matchScrobbles();
		}
		auto maxTimeToHoldUnmatchedScrobbles = std::chrono::days(7);
		auto maxTimeToHoldUnmatchedScrobblesInMemory = std::chrono::minutes(2);
		size_t maxScrobblesToFetch = scrobbler->maxFetchedScrobblesPerRequest();
		auto unmatchedScrobblesList = scrobblerDataIt->second.unmatchedScrobbles.slice(0, maxScrobblesToFetch);
		// get recent scrobbles and match against unmatched scrobbles
		auto promise = ([=]() -> Promise<ScrobbleBatchResult> {
			co_await resumeOnQueue(DispatchQueue::main());
			auto unmatchedScrobbles = LinkedList<UnmatchedScrobble>(unmatchedScrobblesList);
			auto lowerBoundTime = unmatchedScrobbles.front().historyItem->startTime() - std::chrono::minutes(1);
			auto upperBoundTime = unmatchedScrobbles.back().historyItem->startTime() + std::chrono::minutes(1);
			// loop until all scrobbles in date range are fetched
			while(true) {
				// recentTracks is sorted from newest to oldest
				auto recentTracks = co_await scrobbler->getScrobbles({
					.from = lowerBoundTime,
					.to = upperBoundTime,
					.limit = maxScrobblesToFetch
				});
				if(recentTracks.items.empty()) {
					// no tracks in this date range
					break;
				}
				auto recentTracksLowerBound = recentTracks.items.back()->startTime();
				{ // remove any scrobbles that already exist in the DB
					auto recentTracksExists = co_await this->database->hasMatchingScrobbles(recentTracks.items);
					for(size_t i=(recentTracks.items.size() - 1); i != (size_t)-1; i--) {
						bool exists = recentTracksExists[i];
						if(exists) {
							recentTracks.items.removeAt(i);
						}
					}
				}
				// match against existing scrobbles
				LinkedList<Tuple<UnmatchedScrobble,$<Scrobble>>> scrobbleMatchesToCache;
				LinkedList<UnmatchedScrobble> unmatchedScrobblesToDelete;
				LinkedList<UnmatchedScrobble> unmatchedScrobblesToRemoveFromList;
				// iterate through matchingScrobbles from newest to oldest and look for matches in recentTracks
				for(auto& unmatchedScrobble : reversed(unmatchedScrobbles)) {
					// if unmatched scrobble is too far older than any of the items in recentTracks, stop here
					if(unmatchedScrobble.historyItem->startTime() < (recentTracksLowerBound - std::chrono::minutes(1))) {
						break;
					}
					auto lowestMatchDateThreshold = unmatchedScrobble.historyItem->startTime() - std::chrono::minutes(1);
					size_t matchIndex = (size_t)-1;
					TimeInterval matchDateDiff;
					// find matching Scrobble in recentTracks
					for(size_t i=0; i<recentTracks.items.size(); i++) {
						auto& scrobble = recentTracks.items[i];
						if(unmatchedScrobble.matchesInexactly(scrobble)) {
							if(matchIndex != (size_t)-1) {
								// there's already a match for this scrobble, so check if this is a closer match
								auto cmpMatchDateDiff = std::chrono::abs(unmatchedScrobble.historyItem->startTime() - scrobble->startTime());
								if(cmpMatchDateDiff <= matchDateDiff) {
									matchIndex = i;
									matchDateDiff = cmpMatchDateDiff;
								} else {
									// since the date difference is increasing, we're getting further away from the likely match, so stop
									break;
								}
							}
							else {
								matchIndex = i;
								matchDateDiff = std::chrono::abs(unmatchedScrobble.historyItem->startTime() - scrobble->startTime());
							}
						}
						else if(scrobble->startTime() < lowestMatchDateThreshold) {
							break;
						}
					}
					// handle match result
					if(matchIndex == (size_t)-1) {
						// no matching scrobble found
						// if scrobble is older than the max time that we should be holding older UnmatchedScrobbles, then delete it
						auto oldness = Date::now() - unmatchedScrobble.historyItem->startTime();
						if(oldness > maxTimeToHoldUnmatchedScrobbles) {
							// UnmatchedScrobble is too old, so delete it
							unmatchedScrobblesToDelete.pushBack(unmatchedScrobble);
							unmatchedScrobblesToRemoveFromList.pushBack(unmatchedScrobble);
						}
						else if(oldness > maxTimeToHoldUnmatchedScrobblesInMemory) {
							// UnmatchedScrobble is older than 2 minutes, so remove from in-memory match list for now
							unmatchedScrobblesToRemoveFromList.pushBack(unmatchedScrobble);
						}
					}
					else {
						// found a matching scrobble
						auto matchingScrobble = recentTracks.items[matchIndex];
						recentTracks.items.removeAt(matchIndex);
						// found matching scrobble, apply match and give a local ID
						matchingScrobble->matchWith(unmatchedScrobble);
						if(matchingScrobble->_localID.empty()) {
							matchingScrobble->_localID = this->uuidGenerator();
						}
						scrobbleMatchesToCache.pushBack({ unmatchedScrobble, matchingScrobble });
						unmatchedScrobblesToRemoveFromList.pushBack(unmatchedScrobble);
					}
				}
				// cache matching scrobbles
				co_await this->database->replaceUnmatchedScrobbles(scrobbleMatchesToCache);
				// delete unmatched scrobbles that are too old or no longer relevant
				co_await this->database->deleteUnmatchedScrobbles(unmatchedScrobblesToDelete);
				// remove matched/deleted scrobbles from unmatched scrobbles list
				auto scrobblerDataIt = this->scrobblersData.find(scrobblerName);
				if(scrobblerDataIt != this->scrobblersData.end()) {
					for(auto& unmatchedScrobble : unmatchedScrobblesToRemoveFromList) {
						scrobblerDataIt->second.unmatchedScrobbles.removeFirstWhere([&](auto& cmpUnmatchedScrobble) {
							return unmatchedScrobble.equals(cmpUnmatchedScrobble);
						});
					}
				}
				// remove scrobbles outside the date bounds from the unmatchedScrobbles list
				unmatchedScrobbles.removeWhere([=](auto& unmatchedScrobble) {
					return unmatchedScrobble.historyItem->startTime() > (recentTracksLowerBound + std::chrono::minutes(1));
				});
				// check if we could still look for more scrobbles
				if(unmatchedScrobbles.empty()) {
					// no more scrobbles to fetch
					break;
				}
				// update the list of scrobbles being matched
				if(this->matchBatch) {
					this->matchBatch->scrobbles = unmatchedScrobbles;
				}
				// update the upper bound to look for matching scrobbles again in a new range
				if(upperBoundTime == recentTracksLowerBound) {
					// the upper bound already matches the new lower bound, so decrease by a second
					upperBoundTime = recentTracksLowerBound - std::chrono::seconds(1);
				} else {
					upperBoundTime = recentTracksLowerBound;
				}
				// if we're gone through the whole date range, stop here
				if(upperBoundTime <= lowerBoundTime) {
					break;
				}
				// loop again, to look for more matching scrobbles
			}
			// done looping through date range
			// search database for any unmatched scrobbles for each scrobbler
			auto scrobblerStash = this->database->scrobblerStash();
			for(auto scrobbler : scrobblerStash->getScrobblers()) {
				// get scrobbler data
				auto scrobblerName = scrobbler->name();
				auto scrobblerDataIt = this->scrobblersData.find(scrobblerName);
				if(scrobblerDataIt == this->scrobblersData.end()) {
					scrobblerDataIt = std::get<0>(this->scrobblersData.insert(std::make_pair(scrobblerName, ScrobblerData())));
				}
				// if there are already unmatched scrobbles for this scrobbler, ignore and continue
				if(!scrobblerDataIt->second.unmatchedScrobbles.empty()) {
					continue;
				}
				// query unmatched scrobbles from DB
				auto newUnmatchedScrobbles = (co_await this->database->getUnmatchedScrobbles({
					.filters = {
						.scrobbler = scrobblerName
					},
					.range = sql::IndexRange{
						.startIndex = 0,
						.endIndex = maxScrobblesToFetch
					},
					.order = sql::Order::ASC
				})).items;
				if(!newUnmatchedScrobbles.empty()) {
					// we have pending scrobbles, so add them to the list of pending scrobbles
					auto scrobblerDataIt = this->scrobblersData.find(scrobblerName);
					if(scrobblerDataIt == this->scrobblersData.end()) {
						scrobblerDataIt = std::get<0>(this->scrobblersData.insert(std::make_pair(scrobblerName, ScrobblerData())));
					}
					scrobblerDataIt->second.unmatchedScrobbles.pushBackList(newUnmatchedScrobbles);
				}
			}
			co_await resumeOnQueue(DispatchQueue::main());
			// check if there are still any pending scrobbles for scrobblers that are currently able to upload
			bool hasUnmatchedScrobbles = this->scrobblersData.containsWhere([](auto& pair) {
				return !pair.second.unmatchedScrobbles.empty();
			});
			// if there are no pending scrobbles, uploading is finished for now
			co_return hasUnmatchedScrobbles ? ScrobbleBatchResult::HAS_MORE : ScrobbleBatchResult::DONE;
		})();
		// apply current match task
		this->matchBatch = MatchBatch{
			.scrobbler = scrobbler->name(),
			.scrobbles = unmatchedScrobblesList,
			.promise = promise
		};
		// handle promise finishing
		promise.then([=](ScrobbleBatchResult uploadResult) {
			this->matchBatch = std::nullopt;
			if(uploadResult != ScrobbleBatchResult::DONE) {
				matchScrobbles();
			}
		}, [=](std::exception_ptr error) {
			this->matchBatch = std::nullopt;
			console::error("Error fetching scrobbles to match: ", utils::getExceptionDetails(error).fullDescription);
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
		if(historyItem && !currentHistoryItemScrobbled && readyToScrobble(historyItem, finishedItem)) {
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

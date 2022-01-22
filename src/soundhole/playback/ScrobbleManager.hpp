//
//  ScrobbleManager.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/12/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/PlaybackHistoryItem.hpp>
#include <soundhole/media/Scrobble.hpp>
#include <soundhole/media/Scrobbler.hpp>
#include <soundhole/media/UnmatchedScrobble.hpp>
#include "Player.hpp"

namespace sh {
	enum class ScrobbleBatchResult {
		DONE,
		HAS_MORE
	};

	class ScrobbleManager: protected Player::EventListener {
	public:
		ScrobbleManager(MediaDatabase* database);
		~ScrobbleManager();
		
		void setPlayer(Player*);

		bool readyToScrobble($<PlaybackHistoryItem> historyItem, bool finishedItem) const;
		void scrobble($<PlaybackHistoryItem>);
		
		ArrayList<$<Scrobble>> getUploadingScrobbles();
		ArrayList<$<const Scrobble>> getUploadingScrobbles() const;
		bool isUploadingScrobbles() const;
		Promise<ScrobbleBatchResult> uploadScrobbles();
		
		ArrayList<UnmatchedScrobble> getMatchingScrobbles() const;
		bool isMatchingScrobbles() const;
		Promise<ScrobbleBatchResult> matchScrobbles();
		
	protected:
		virtual void onPlayerStateChange($<Player> player, const Player::Event& event) override;
		virtual void onPlayerMetadataChange($<Player> player, const Player::Event& event) override;
		virtual void onPlayerTrackFinish($<Player> player, const Player::Event& event) override;
		
		virtual void onPlayerPlay($<Player> player, const Player::Event& event) override;
		virtual void onPlayerPause($<Player> player, const Player::Event& event) override;
		
	private:
		static Function<String()> createUUIDGenerator();
		$<Scrobble> createScrobble(Scrobbler* scrobbler, $<PlaybackHistoryItem> historyItem, $<Album> album);
		
		void updateFromPlayer($<Player> player, bool finishedItem);
		
		Player* player;
		MediaDatabase* database;
		
		Function<String()> uuidGenerator;
		
		$<PlaybackHistoryItem> currentHistoryItem;
		bool currentHistoryItemScrobbled;
		
		struct ScrobblerData {
			LinkedList<$<Scrobble>> pendingScrobbles;
			LinkedList<UnmatchedScrobble> unmatchedScrobbles;
			Optional<Date> dailyScrobbleLimitExceededDate;
			
			bool currentlyAbleToUpload() const;
			bool dailyScrobbleLimitExceeded() const;
		};
		Map<String,ScrobblerData> scrobblersData;
		
		struct UploadBatch {
			String scrobbler;
			ArrayList<$<Scrobble>> scrobbles;
			Promise<ScrobbleBatchResult> promise;
		};
		Optional<UploadBatch> uploadBatch;
		
		struct MatchBatch {
			String scrobbler;
			ArrayList<UnmatchedScrobble> scrobbles;
			Promise<ScrobbleBatchResult> promise;
		};
		Optional<MatchBatch> matchBatch;
	};
}

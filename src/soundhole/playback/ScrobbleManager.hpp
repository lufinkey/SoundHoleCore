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
#include "Player.hpp"

namespace sh {
	enum class ScrobbleUploadResult {
		DONE,
		HAS_MORE_UPLOADS
	};

	class ScrobbleManager: protected Player::EventListener {
	public:
		ScrobbleManager(Player* player, MediaDatabase* database);
		~ScrobbleManager();
		
		void scrobble($<PlaybackHistoryItem>);
		
		ArrayList<$<Scrobble>> getUploadingScrobbles();
		ArrayList<$<const Scrobble>> getUploadingScrobbles() const;
		
		bool isUploadingScrobbles() const;
		Promise<ScrobbleUploadResult> uploadPendingScrobbles();
		
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
		
		struct UnmatchedScrobble {
			Date startTime;
			String trackURI;
		};
		
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
			Promise<ScrobbleUploadResult> promise;
		};
		Optional<UploadBatch> uploadBatch;
	};
}

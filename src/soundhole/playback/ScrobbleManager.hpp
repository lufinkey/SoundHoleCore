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
	class ScrobbleManager: protected Player::EventListener {
	public:
		ScrobbleManager(Player* player, MediaDatabase* database);
		~ScrobbleManager();
		
		void scrobble($<PlaybackHistoryItem>);
		
		ArrayList<$<Scrobble>> getUploadingScrobbles();
		ArrayList<$<const Scrobble>> getUploadingScrobbles() const;
		
		bool isUploadingScrobbles() const;
		/// Uploads any pending scrobbles.
		/// If `false` is returned, there are still more scrobbles to be uploaded.
		/// If `true` is returned, all pending scrobbles have been uploaded
		Promise<bool> uploadPendingScrobbles();
		
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
		};
		Map<String,ScrobblerData> scrobblersData;
		
		struct UploadBatch {
			String scrobbler;
			ArrayList<$<Scrobble>> scrobbles;
			Promise<bool> promise;
		};
		Optional<UploadBatch> uploadBatch;
	};
}

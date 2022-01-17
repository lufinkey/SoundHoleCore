//
//  PlayerHistoryManager.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/30/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/PlaybackHistoryItem.hpp>
#include <soundhole/database/MediaDatabase.hpp>
#include "Player.hpp"

namespace sh {
	class PlayerHistoryManager: protected Player::EventListener {
		friend class Player;
	public:
		PlayerHistoryManager(MediaDatabase* database);
		~PlayerHistoryManager();
		
		void setPlayer(Player*);
		
		$<PlaybackHistoryItem> currentItem() const;
		
	protected:
		virtual void onPlayerStateChange($<Player> player, const Player::Event& event) override;
		virtual void onPlayerMetadataChange($<Player> player, const Player::Event& event) override;
		virtual void onPlayerTrackFinish($<Player> player, const Player::Event& event) override;
		
		virtual void onPlayerPlay($<Player> player, const Player::Event& event) override;
		virtual void onPlayerPause($<Player> player, const Player::Event& event) override;
		
	private:
		using SteadyClock = std::chrono::steady_clock;
		
		void updateFromPlayer($<Player> player, bool finishedItem);
		void deleteFromHistory($<PlaybackHistoryItem> item);
		bool canSaveItem($<PlaybackHistoryItem>) const;
		void updateHistoryDBIfNeeded($<PlaybackHistoryItem> item, bool finishedItem);
		
		Player* player;
		MediaDatabase* database;
		
		$<PlaybackHistoryItem> historyItem;
		Optional<double> position;
		SteadyClock::time_point positionTimePoint;
		bool wasPlaying;
		
		$<PlaybackHistoryItem> restartedHistoryItem;
		$<PlaybackHistoryItem> nonRestartedHistoryItem;
	};
}

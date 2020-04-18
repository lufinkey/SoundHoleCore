//
//  Player_objc_private.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 4/18/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#ifdef __OBJC__
#include "Player_objc.hpp"

namespace sh {
	class SHPlayerEventListenerWrapper: public Player::EventListener {
	public:
		SHPlayerEventListenerWrapper(id<SHPlayerEventListener> listener);
		
		id<SHPlayerEventListener> getObjCListener() const;
		
		virtual void onPlayerStateChange($<Player> player, const Player::Event& event) override;
		virtual void onPlayerMetadataChange($<Player> player, const Player::Event& event) override;
		virtual void onPlayerQueueChange($<Player> player, const Player::Event& event) override;
		virtual void onPlayerTrackFinish($<Player> player, const Player::Event& event) override;
		
		virtual void onPlayerPlay($<Player> player, const Player::Event& event) override;
		virtual void onPlayerPause($<Player> player, const Player::Event& event) override;
		
	private:
		id<SHPlayerEventListener> objcListener;
	};
}

#else
namespace sh {
	class SHPlayerEventListenerWrapper;
}
#endif

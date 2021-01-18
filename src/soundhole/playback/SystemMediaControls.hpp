//
//  SystemMediaControls.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/23/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "MediaControls.hpp"

namespace sh {
	struct _SystemMediaControlsNativeData;
	struct _SystemMediaControlsListenerNode;

	class SystemMediaControls: public MediaControls {
	public:
		static SystemMediaControls* shared();
		~SystemMediaControls();
		
		virtual void addListener(Listener*) override;
		virtual void removeListener(Listener*) override;
		
		virtual void setButtonState(ButtonState) override;
		virtual ButtonState getButtonState() const override;
		
		virtual void setPlaybackState(PlaybackState) override;
		virtual PlaybackState getPlaybackState() const override;
		
		virtual void setNowPlaying(NowPlaying) override;
		virtual NowPlaying getNowPlaying() const override;
		
	private:
		SystemMediaControls();
		SystemMediaControls(const SystemMediaControls&) = delete;
		SystemMediaControls& operator=(const SystemMediaControls&) = delete;
		
		_SystemMediaControlsNativeData* nativeData;
		LinkedList<_SystemMediaControlsListenerNode*> listenerNodes;
	};
}

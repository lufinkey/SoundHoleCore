//
//  YoutubeAuthEventListener.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/12/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class YoutubeAuth;
	
	class YoutubeAuthEventListener {
	public:
		virtual ~YoutubeAuthEventListener() {}
		
		virtual void onYoutubeAuthSessionResume(YoutubeAuth* auth) {}
		virtual void onYoutubeAuthSessionStart(YoutubeAuth* auth) {}
		virtual void onYoutubeAuthSessionRenew(YoutubeAuth* auth) {}
		virtual void onYoutubeAuthSessionExpire(YoutubeAuth* auth) {}
		virtual void onYoutubeAuthSessionEnd(YoutubeAuth* auth) {}
	};
}

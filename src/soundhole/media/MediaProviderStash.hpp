//
//  MediaProviderStash.hpp
//  SoundHoleCore-iOS
//
//  Created by Luis Finke on 2/27/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class MediaItem;
	class MediaProvider;

	class MediaProviderStash {
	public:
		virtual ~MediaProviderStash() {}
		
		virtual $<MediaItem> parseMediaItem(const Json& json);
		virtual MediaProvider* getMediaProvider(const String& name) = 0;
	};
}

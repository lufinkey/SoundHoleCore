//
//  MediaProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/MediaItem.hpp>

namespace sh {
	class MediaProvider {
	public:
		MediaProvider(const MediaProvider&) = delete;
		MediaProvider& operator=(const MediaProvider&) = delete;
		
		MediaProvider() {};
		virtual ~MediaProvider() {}
		
		virtual String getName() const = 0;
		virtual String getDisplayName() const = 0;
	};
}

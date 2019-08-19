//
//  MediaItem.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/external.hpp>

namespace sh {
	class MediaProvider;
	
	class MediaItem {
	public:
		MediaItem(MediaProvider* provider);
		
		virtual String getType() const = 0;
		
		virtual String getName() const = 0;
		virtual String getURI() const = 0;
		
		virtual MediaProvider* getProvider() const;
		
	private:
		MediaProvider* provider;
	};
}

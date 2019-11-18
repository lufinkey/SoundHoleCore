//
//  Artist.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "MediaItem.hpp"

namespace sh {
	class Artist: public MediaItem {
	public:
		using MediaItem::MediaItem;
		
		virtual Optional<String> description() const = 0;
	};
}

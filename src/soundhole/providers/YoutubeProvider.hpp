//
//  YoutubeProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/23/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/providers/MediaProvider.hpp>

namespace sh {
	class YoutubeProvider: public MediaProvider {
	public:
		YoutubeProvider();
		
		virtual String getName() const override;
		virtual String getDisplayName() const override;
	};
}

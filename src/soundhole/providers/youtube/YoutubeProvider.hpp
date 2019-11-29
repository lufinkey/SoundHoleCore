//
//  YoutubeProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/23/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/media/MediaProvider.hpp>

namespace sh {
	class YoutubeProvider: public MediaProvider {
	public:
		YoutubeProvider();
		
		virtual String name() const override;
		virtual String displayName() const override;
	};
}

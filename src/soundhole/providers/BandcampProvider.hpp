//
//  BandcampProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <memory>
#include <soundhole/providers/MediaProvider.hpp>

namespace sh {
	class BandcampProvider: public MediaProvider {
	public:
		virtual String getName() const override;
		virtual String getDisplayName() const override;
		
		
		struct SearchOptions {
			size_t page;
		};
		struct SearchResults {
			size_t page;
			LinkedList<std::shared_ptr<MediaItem*>> items;
		};
		virtual Promise<SearchResults> search(String text, SearchOptions options) const;
	};
}

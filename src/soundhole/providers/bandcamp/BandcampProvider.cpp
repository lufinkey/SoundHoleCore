//
//  BandcampProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "BandcampProvider.hpp"

namespace sh {
	String BandcampProvider::getName() const {
		return "bandcamp";
	}
	
	String BandcampProvider::getDisplayName() const {
		return "Bandcamp";
	}
	
	Promise<BandcampProvider::SearchResults> BandcampProvider::search(String text, SearchOptions options) const {
		// TODO implement search
		return Promise<SearchResults>::resolve(SearchResults{ .page = 0 });
	}
}

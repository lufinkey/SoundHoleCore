//
//  Artist.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "Artist.hpp"

namespace sh {
	$<Artist> Artist::new$(MediaProvider* provider, Data data) {
		return fgl::new$<Artist>(provider, data);
	}

	Artist::Artist(MediaProvider* provider, Data data)
	: MediaItem(provider, data),
	_description(data.description) {
		//
	}
	
	const Optional<String>& Artist::description() const {
		return _description;
	}



	bool Artist::needsData() const {
		// TODO implement needsData
		return false;
	}

	Promise<void> fetchMissingData() {
		// TODO implement fetchMissingData
		return Promise<void>::reject(std::runtime_error("not implemented"));
	}
}

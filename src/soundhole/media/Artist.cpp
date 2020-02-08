//
//  Artist.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "Artist.hpp"
#include "MediaProvider.hpp"

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
		// TODO implement Artist::needsData
		return false;
	}

	Promise<void> Artist::fetchMissingData() {
		return provider->getArtistData(_uri).then([=](Data data) {
			_name = data.name;
			_description = data.description;
		});
	}



	Artist::Data Artist::toData() const {
		return Artist::Data{
			MediaItem::toData(),
			.description=_description
		};
	}
}

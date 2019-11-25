//
//  Album.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "Album.hpp"

namespace sh {
	Album::Album(MediaProvider* provider, Data data)
	: SpecialTrackCollection<AlbumItem>(provider, data),
	_artists(data.artists) {
		//
	}
	
	const ArrayList<$<Artist>>& Album::artists() {
		return _artists;
	}

	const ArrayList<$<const Artist>>& Album::artists() const {
		return *((const ArrayList<$<const Artist>>*)(&_artists));
	}
}

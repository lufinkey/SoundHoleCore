//
//  Playlist.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "TrackCollection.hpp"

namespace sh {
	class PlaylistItem: public TrackCollectionItem {
		//
	};


	class Playlist: public TrackCollection {
	public:
		using TrackCollection::TrackCollection;
	};
}

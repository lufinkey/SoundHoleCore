//
//  Album.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "TrackCollection.hpp"

namespace sh {
	class AlbumItem: public TrackCollectionItem {
		//
	};


	class Album: public TrackCollection {
	public:
		using TrackCollection::TrackCollection;
		
		virtual ArrayList<$<Artist>> artists() = 0;
		virtual ArrayList<$<const Artist>> artists() const = 0;
	};
}

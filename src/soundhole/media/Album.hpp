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
	class Album;


	class AlbumItem: public SpecialTrackCollectionItem<Album> {
	public:
		using SpecialTrackCollectionItem<Album>::SpecialTrackCollectionItem;
	};


	class Album: public SpecialTrackCollection<AlbumItem> {
	public:
		using SpecialTrackCollection<AlbumItem>::SpecialTrackCollection;
		
		virtual ArrayList<$<Artist>> artists() = 0;
		virtual ArrayList<$<const Artist>> artists() const = 0;
	};
}

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
	class Playlist;


	class PlaylistItem: public SpecialTrackCollectionItem<Playlist> {
	public:
		using SpecialTrackCollectionItem<Playlist>::SpecialTrackCollectionItem;
		
		static $<PlaylistItem> new$($<Playlist> album, Data data);
		static $<PlaylistItem> new$($<SpecialTrackCollection<PlaylistItem>> album, Data data);
	};


	class Playlist: public SpecialTrackCollection<PlaylistItem> {
	public:
		using SpecialTrackCollection<PlaylistItem>::SpecialTrackCollection;
	};
}

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
		static $<AlbumItem> new$($<Album> album, Data data);
		static $<AlbumItem> new$($<SpecialTrackCollection<AlbumItem>> album, Data data);
		
		AlbumItem($<Album> album, Data data);
		
		virtual bool matchesItem(const TrackCollectionItem* item) const override;
	};


	class Album: public SpecialTrackCollection<AlbumItem> {
	public:
		struct Data: public SpecialTrackCollection<AlbumItem>::Data {
			ArrayList<$<Artist>> artists;
		};
		
		Album(MediaProvider* provider, Data data);
		
		const ArrayList<$<Artist>>& artists();
		const ArrayList<$<const Artist>>& artists() const;
		
	private:
		ArrayList<$<Artist>> _artists;
	};
}

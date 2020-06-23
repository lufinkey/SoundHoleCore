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
		static $<AlbumItem> new$($<SpecialTrackCollection<AlbumItem>> album, Data data);
		
		AlbumItem($<SpecialTrackCollection<AlbumItem>> album, Data data);
		AlbumItem($<SpecialTrackCollection<AlbumItem>> album, Json json, MediaProviderStash* stash);
		
		virtual bool matchesItem(const TrackCollectionItem* item) const override;
		
		static $<AlbumItem> fromJson($<SpecialTrackCollection<AlbumItem>> album, Json json, MediaProviderStash* stash);
	};


	class Album: public SpecialTrackCollection<AlbumItem> {
	public:
		struct Data: public SpecialTrackCollection<AlbumItem>::Data {
			ArrayList<$<Artist>> artists;
		};
		
		static $<Album> new$(MediaProvider* provider, Data data);
		
		Album(MediaProvider* provider, Data data);
		Album(Json json, MediaProviderStash* stash);
		
		const ArrayList<$<Artist>>& artists();
		const ArrayList<$<const Artist>>& artists() const;
		
		virtual Promise<void> fetchData() override;
		void applyData(const Data& data);
		
		Data toData(DataOptions options = DataOptions()) const;
		
		static $<Album> fromJson(Json json, MediaProviderStash* stash);
		virtual Json toJson(ToJsonOptions options) const override;
		
	protected:
		virtual MutatorDelegate* createMutatorDelegate() override;
		
		ArrayList<$<Artist>> _artists;
	};
}

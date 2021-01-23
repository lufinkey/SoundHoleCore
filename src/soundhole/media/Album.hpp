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
		static $<AlbumItem> new$($<SpecialTrackCollection<AlbumItem>> album, const Data& data);
		AlbumItem($<SpecialTrackCollection<AlbumItem>> album, const Data& data);
		
		virtual bool matchesItem(const TrackCollectionItem* item) const override;
	};


	class Album: public SpecialTrackCollection<AlbumItem> {
	public:
		struct Data: public SpecialTrackCollection<AlbumItem>::Data {
			ArrayList<$<Artist>> artists;
			
			static Data fromJson(const Json& json, MediaProviderStash* stash);
		};
		
		static $<Album> new$(MediaProvider* provider, const Data& data);
		
		Album(MediaProvider* provider, const Data& data);
		
		const ArrayList<$<Artist>>& artists();
		const ArrayList<$<const Artist>>& artists() const;
		
		virtual Promise<void> fetchData() override;
		void applyData(const Data& data);
		
		Data toData(const DataOptions& options = DataOptions()) const;
		virtual Json toJson(const ToJsonOptions& options) const override;
		
	protected:
		virtual MutatorDelegate* createMutatorDelegate() override;
		
		ArrayList<$<Artist>> _artists;
	};
}

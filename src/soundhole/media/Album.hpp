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
		
		virtual bool matchesItem(const TrackCollectionItem* item) const override;
		
		static $<AlbumItem> fromJson($<SpecialTrackCollection<AlbumItem>> album, Json json, MediaProviderStash* stash);
		
	protected:
		AlbumItem($<TrackCollectionItem>& ptr, $<SpecialTrackCollection<AlbumItem>> album, Data data);
		AlbumItem($<TrackCollectionItem>& ptr, $<SpecialTrackCollection<AlbumItem>> album, Json json, MediaProviderStash* stash);
	};


	class Album: public SpecialTrackCollection<AlbumItem> {
	public:
		struct Data: public SpecialTrackCollection<AlbumItem>::Data {
			ArrayList<$<Artist>> artists;
		};
		
		static $<Album> new$(MediaProvider* provider, Data data);
		
		const ArrayList<$<Artist>>& artists();
		const ArrayList<$<const Artist>>& artists() const;
		
		virtual bool needsData() const override;
		virtual Promise<void> fetchMissingData() override;
		
		Data toData(DataOptions options = DataOptions()) const;
		
		static $<Album> fromJson(Json json, MediaProviderStash* stash);
		virtual Json toJson(ToJsonOptions options) const override;
		
	protected:
		Album($<MediaItem>& ptr, MediaProvider* provider, Data data);
		Album($<MediaItem>& ptr, Json json, MediaProviderStash* stash);
		
		virtual MutatorDelegate* createMutatorDelegate() override;
		
		ArrayList<$<Artist>> _artists;
	};
}

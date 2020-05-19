//
//  ShuffledTrackCollection.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/25/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "TrackCollection.hpp"

namespace sh {
	class ShuffledTrackCollection;


	class ShuffledTrackCollectionItem: public SpecialTrackCollectionItem<ShuffledTrackCollection> {
	public:
		struct Data: public SpecialTrackCollectionItem<ShuffledTrackCollection>::Data {
			$<TrackCollectionItem> sourceItem;
		};
		
		static $<ShuffledTrackCollectionItem> new$($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, Data data);
		static $<ShuffledTrackCollectionItem> new$($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, $<TrackCollectionItem> sourceItem);
		
		$<TrackCollectionItem> sourceItem();
		$<const TrackCollectionItem> sourceItem() const;
		
		virtual bool matchesItem(const TrackCollectionItem* item) const override;
		
		static $<ShuffledTrackCollectionItem> fromJson($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, Json json, MediaProviderStash* stash);
		virtual Json toJson() const override;
		
	protected:
		ShuffledTrackCollectionItem($<TrackCollectionItem>& ptr, $<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, Data data);
		ShuffledTrackCollectionItem($<TrackCollectionItem>& ptr, $<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, $<TrackCollectionItem> sourceItem);
		ShuffledTrackCollectionItem($<TrackCollectionItem>& ptr, $<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, Json json, MediaProviderStash* stash);
		
		$<TrackCollectionItem> _sourceItem;
	};


	class ShuffledTrackCollection: public SpecialTrackCollection<ShuffledTrackCollectionItem>, public SpecialTrackCollection<ShuffledTrackCollectionItem>::MutatorDelegate {
	public:
		static $<ShuffledTrackCollection> new$($<TrackCollection> source, ArrayList<$<TrackCollectionItem>> initialItems={});
		
		$<TrackCollection> source();
		$<const TrackCollection> source() const;
		
		virtual Promise<void> fetchData() override;
		
		virtual Json toJson(ToJsonOptions options) const override;
		
	protected:
		ShuffledTrackCollection($<MediaItem>& ptr, $<TrackCollection> source, ArrayList<$<TrackCollectionItem>> initialItems = {});
		
		struct RandomIndex {
			size_t index;
			
			RandomIndex(size_t index): index(index) {}
		};
		
		virtual MutatorDelegate* createMutatorDelegate() override;
		virtual Promise<void> loadItems(Mutator* mutator, size_t index, size_t count) override;
		
		$<TrackCollection> _source;
		LinkedList<$<RandomIndex>> _remainingIndexes;
	};
}

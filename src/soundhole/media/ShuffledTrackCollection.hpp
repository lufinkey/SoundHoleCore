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
		
		static $<ShuffledTrackCollectionItem> new$($<ShuffledTrackCollection> context, Data data);
		static $<ShuffledTrackCollectionItem> new$($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, Data data);
		static $<ShuffledTrackCollectionItem> new$($<ShuffledTrackCollection> context, $<TrackCollectionItem> sourceItem);
		
		ShuffledTrackCollectionItem($<ShuffledTrackCollection> context, Data data);
		ShuffledTrackCollectionItem($<ShuffledTrackCollection> context, $<TrackCollectionItem> sourceItem);
		
		$<TrackCollectionItem> sourceItem();
		$<const TrackCollectionItem> sourceItem() const;
		
		virtual bool matchesItem(const TrackCollectionItem* item) const override;
		
	protected:
		$<TrackCollectionItem> _sourceItem;
	};


	class ShuffledTrackCollection: public SpecialTrackCollection<ShuffledTrackCollectionItem>, protected SpecialTrackCollection<ShuffledTrackCollectionItem>::MutatorDelegate {
	public:
		static $<ShuffledTrackCollection> new$($<TrackCollection> source, ArrayList<$<TrackCollectionItem>> initialItems={});
		
		ShuffledTrackCollection($<TrackCollection> source, ArrayList<$<TrackCollectionItem>> initialItems = {});
		
		$<TrackCollection> source();
		$<const TrackCollection> source() const;
		
		virtual bool needsData() const override;
		virtual Promise<void> fetchMissingData() override;
		
	protected:
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

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
			
			static Data forSourceItem($<TrackCollectionItem> sourceItem);
		};
		
		static $<ShuffledTrackCollectionItem> new$($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, const Data& data);
		ShuffledTrackCollectionItem($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, const Data& data);
		
		$<TrackCollectionItem> sourceItem();
		$<const TrackCollectionItem> sourceItem() const;
		
		virtual bool matchesItem(const TrackCollectionItem* item) const override;
		
		virtual Json toJson() const override;
		
	protected:
		$<TrackCollectionItem> _sourceItem;
	};


	class ShuffledTrackCollection: public SpecialTrackCollection<ShuffledTrackCollectionItem>, public SpecialTrackCollection<ShuffledTrackCollectionItem>::MutatorDelegate {
	public:
		using LoadItemOptions = TrackCollection::LoadItemOptions;
		
		static $<ShuffledTrackCollection> new$($<TrackCollection> source, ArrayList<$<TrackCollectionItem>> initialItems={});
		ShuffledTrackCollection($<TrackCollection> source, ArrayList<$<TrackCollectionItem>> initialItems = {});
		virtual ~ShuffledTrackCollection();
		
		virtual String versionId() const override;
		
		$<TrackCollection> source();
		$<const TrackCollection> source() const;
		
		virtual Promise<void> fetchData() override;
		
		virtual Json toJson(const ToJsonOptions& options) const override;
		
	protected:
		virtual MutatorDelegate* createMutatorDelegate() override;
		virtual size_t getChunkSize() const override;
		virtual Promise<void> loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) override;
		virtual Promise<void> insertItems(Mutator* mutator, size_t index, LinkedList<$<Track>> tracks) override;
		virtual Promise<void> appendItems(Mutator* mutator, LinkedList<$<Track>> tracks) override;
		virtual Promise<void> removeItems(Mutator* mutator, size_t index, size_t count) override;
		virtual Promise<void> moveItems(Mutator* mutator, size_t index, size_t count, size_t newIndex) override;
		
		size_t shuffledListSize() const;
		
		$<TrackCollection> _source;
		LinkedList<ItemIndexMarker> _remainingIndexes;
		void filterRemainingIndexes();
	};
}

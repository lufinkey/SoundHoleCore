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
		
		static $<ShuffledTrackCollectionItem> new$($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, const Data& data);
		static $<ShuffledTrackCollectionItem> new$($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, $<TrackCollectionItem> sourceItem);
		
		ShuffledTrackCollectionItem($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, const Data& data);
		ShuffledTrackCollectionItem($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, $<TrackCollectionItem> sourceItem);
		ShuffledTrackCollectionItem($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, const Json& json, MediaProviderStash* stash);
		
		$<TrackCollectionItem> sourceItem();
		$<const TrackCollectionItem> sourceItem() const;
		
		virtual bool matchesItem(const TrackCollectionItem* item) const override;
		
		static $<ShuffledTrackCollectionItem> fromJson($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, const Json& json, MediaProviderStash* stash);
		virtual Json toJson() const override;
		
	protected:
		$<TrackCollectionItem> _sourceItem;
	};


	class ShuffledTrackCollection: public SpecialTrackCollection<ShuffledTrackCollectionItem>, public SpecialTrackCollection<ShuffledTrackCollectionItem>::MutatorDelegate {
	public:
		using LoadItemOptions = TrackCollection::LoadItemOptions;
		
		static $<ShuffledTrackCollection> new$($<TrackCollection> source, ArrayList<$<TrackCollectionItem>> initialItems={});
		ShuffledTrackCollection($<TrackCollection> source, ArrayList<$<TrackCollectionItem>> initialItems = {});
		
		virtual String versionId() const override;
		
		$<TrackCollection> source();
		$<const TrackCollection> source() const;
		
		virtual Promise<void> fetchData() override;
		
		virtual Json toJson(const ToJsonOptions& options) const override;
		
	protected:
		struct RandomIndex {
			size_t index;
			
			RandomIndex(size_t index): index(index) {}
		};
		
		virtual MutatorDelegate* createMutatorDelegate() override;
		virtual Promise<void> loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) override;
		
		$<TrackCollection> _source;
		LinkedList<$<RandomIndex>> _remainingIndexes;
	};
}

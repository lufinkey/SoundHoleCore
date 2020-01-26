//
//  ShuffledTrackCollection.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/25/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "ShuffledTrackCollection.hpp"
#include <cmath>
#include <cstdlib>

namespace sh {
	$<ShuffledTrackCollectionItem> ShuffledTrackCollectionItem::new$($<ShuffledTrackCollection> context, Data data) {
		return fgl::new$<ShuffledTrackCollectionItem>(context, data);
	}

	$<ShuffledTrackCollectionItem> ShuffledTrackCollectionItem::new$($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, Data data) {
		return fgl::new$<ShuffledTrackCollectionItem>(std::static_pointer_cast<ShuffledTrackCollection>(context), data);
	}

	$<ShuffledTrackCollectionItem> ShuffledTrackCollectionItem::new$($<ShuffledTrackCollection> context, $<TrackCollectionItem> sourceItem) {
		return fgl::new$<ShuffledTrackCollectionItem>(context, sourceItem);
	}
	
	ShuffledTrackCollectionItem::ShuffledTrackCollectionItem($<ShuffledTrackCollection> context, Data data)
	: SpecialTrackCollectionItem<ShuffledTrackCollection>(context, data), _sourceItem(data.sourceItem) {
		//
	}

	ShuffledTrackCollectionItem::ShuffledTrackCollectionItem($<ShuffledTrackCollection> context, $<TrackCollectionItem> sourceItem)
	: SpecialTrackCollectionItem<ShuffledTrackCollection>(context, {.track=sourceItem->track()}), _sourceItem(sourceItem) {
		//
	}
	
	$<TrackCollectionItem> ShuffledTrackCollectionItem::sourceItem() {
		return _sourceItem;
	}

	$<const TrackCollectionItem> ShuffledTrackCollectionItem::sourceItem() const {
		return std::static_pointer_cast<const TrackCollectionItem>(_sourceItem);
	}
	
	bool ShuffledTrackCollectionItem::matchesItem(const TrackCollectionItem* item) const {
		if(auto shuffledItem = dynamic_cast<const ShuffledTrackCollectionItem*>(item)) {
			return _sourceItem->matchesItem(shuffledItem->sourceItem().get());
		}
		return false;
	}



	ShuffledTrackCollection::ShuffledTrackCollection($<TrackCollection> source, ArrayList<$<TrackCollectionItem>> initialItems)
	: SpecialTrackCollection<ShuffledTrackCollectionItem>(source->mediaProvider(), SpecialTrackCollection<ShuffledTrackCollectionItem>::Data{{
		.type="shuffled-collection",
		.name=source->name(),
		.uri=source->uri(),
		.images=std::nullopt
		},
		.tracks=SpecialTrackCollection<ShuffledTrackCollectionItem>::Data::Tracks{
			.total=source->itemCount().value_or(initialItems.size()),
			.offset=0,
			.items=initialItems.map<ShuffledTrackCollectionItem::Data>([&](auto& item) {
				return ShuffledTrackCollectionItem::Data{{
					.track=item->track(),
					},
					.sourceItem=item
				};
			})
		}
	}), _source(source) {
		FGL_ASSERT(std::dynamic_pointer_cast<ShuffledTrackCollection>(source) != nullptr, "Cannot create ShuffledTrackCollection with another ShuffledTrackCollection");
		
		// get indexes of initialItems
		LinkedList<size_t> initialIndexes = {};
		for(auto item : initialItems) {
			auto index = source->indexOfItemInstance(item.get());
			FGL_ASSERT(index.has_value(), "initialItems must only contains items from the given source collection");
			initialIndexes.pushBack(index.value());
		}
		
		// build list of remaining indexes
		LinkedList<size_t> indexes;
		size_t itemCount = this->itemCount().value_or(0);
		for(size_t i=0; i<itemCount; i++) {
			auto it = initialIndexes.findEqual(i);
			if(it != initialIndexes.end()) {
				initialIndexes.erase(it);
			} else {
				indexes.pushBack(*it);
			}
		}
		
		// randomize list of remaining indexes
		while(indexes.size() > 0) {
			size_t index = (size_t)(((double)std::rand() / (double)RAND_MAX) * (double)indexes.size());
			auto it = std::next(indexes.begin(), index);
			size_t randomIndex = *it;
			_remainingIndexes.pushBack(fgl::new$<RandomIndex>(randomIndex));
			indexes.erase(it);
		}
		
		// TODO subscribe to changes in source
	}

	ShuffledTrackCollection::MutatorDelegate* ShuffledTrackCollection::createMutatorDelegate() {
		return this;
	}

	Promise<void> ShuffledTrackCollection::loadItems(Mutator* mutator, size_t index, size_t count) {
		auto self = std::static_pointer_cast<ShuffledTrackCollection>(this->self());
		size_t endIndex = index+count;
		$<LinkedList<$<ShuffledTrackCollectionItem>>> items;
		auto promise = Promise<void>::resolve();
		auto tmpRemainingIndexes = _remainingIndexes;
		auto chosenIndexes = LinkedList<$<RandomIndex>>();
		for(size_t i=index; i<endIndex; i++) {
			auto existingItem = itemAt(i);
			if(existingItem) {
				promise.then([=]() {
					items->pushBack(ShuffledTrackCollectionItem::new$(self, existingItem));
				});
			} else {
				if(tmpRemainingIndexes.empty()) {
					break;
				}
				auto randomIndex = tmpRemainingIndexes.extractFront();
				chosenIndexes.pushBack(randomIndex);
				promise = promise.then([=]() {
					return _source->getItem(randomIndex->index).then([=]($<TrackCollectionItem> item) {
						items->pushBack(ShuffledTrackCollectionItem::new$(self, item));
					});
				});
			}
		}
		return promise.then([=]() {
			for(auto randomIndex : chosenIndexes) {
				auto it = _remainingIndexes.findEqual(randomIndex);
				if(it != _remainingIndexes.end()) {
					_remainingIndexes.erase(it);
				}
			}
			mutator->apply(index, *items);
		});
	}
}

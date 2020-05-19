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
	$<ShuffledTrackCollectionItem> ShuffledTrackCollectionItem::new$($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, Data data) {
		$<TrackCollectionItem> ptr;
		new ShuffledTrackCollectionItem(ptr, context, data);
		return std::static_pointer_cast<ShuffledTrackCollectionItem>(ptr);
	}
	
	$<ShuffledTrackCollectionItem> ShuffledTrackCollectionItem::new$($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, $<TrackCollectionItem> sourceItem) {
		$<TrackCollectionItem> ptr;
		new ShuffledTrackCollectionItem(ptr, context, sourceItem);
		return std::static_pointer_cast<ShuffledTrackCollectionItem>(ptr);
	}
	
	ShuffledTrackCollectionItem::ShuffledTrackCollectionItem($<TrackCollectionItem>& ptr, $<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, Data data)
	: SpecialTrackCollectionItem<ShuffledTrackCollection>(ptr, context, data), _sourceItem(data.sourceItem) {
		//
	}
	
	ShuffledTrackCollectionItem::ShuffledTrackCollectionItem($<TrackCollectionItem>& ptr, $<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, $<TrackCollectionItem> sourceItem)
	: SpecialTrackCollectionItem<ShuffledTrackCollection>(ptr, context, {.track=sourceItem->track()}), _sourceItem(sourceItem) {
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

	$<ShuffledTrackCollectionItem> ShuffledTrackCollectionItem::fromJson($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, Json json, MediaProviderStash* stash) {
		$<TrackCollectionItem> ptr;
		new ShuffledTrackCollectionItem(ptr, context, json, stash);
		return std::static_pointer_cast<ShuffledTrackCollectionItem>(ptr);
	}

	ShuffledTrackCollectionItem::ShuffledTrackCollectionItem($<TrackCollectionItem>& ptr, $<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, Json json, MediaProviderStash* stash)
	: SpecialTrackCollectionItem<ShuffledTrackCollection>(ptr, context, json, stash) {
		auto shuffledContext = std::static_pointer_cast<ShuffledTrackCollection>(context);
		auto sourceContext = shuffledContext->source();
		_sourceItem = sourceContext->itemFromJson(json["sourceItem"], stash);
		_track = _sourceItem->track();
	}

	Json ShuffledTrackCollectionItem::toJson() const {
		auto sourceIndex = _sourceItem->indexInContext();
		return Json::object{
			{ "sourceItem", _sourceItem->toJson() },
			{ "sourceIndex", sourceIndex ? Json((double)sourceIndex.value()) : Json() }
		};
	}




	$<ShuffledTrackCollection> ShuffledTrackCollection::new$($<TrackCollection> source, ArrayList<$<TrackCollectionItem>> initialItems) {
		$<MediaItem> ptr;
		new ShuffledTrackCollection(ptr, source, initialItems);
		return std::static_pointer_cast<ShuffledTrackCollection>(ptr);
	}

	ShuffledTrackCollection::ShuffledTrackCollection($<MediaItem>& ptr, $<TrackCollection> source, ArrayList<$<TrackCollectionItem>> initialItems)
	: SpecialTrackCollection<ShuffledTrackCollectionItem>(ptr, source->mediaProvider(), SpecialTrackCollection<ShuffledTrackCollectionItem>::Data{{
		.partial=source->needsData(),
		.type="shuffled-collection",
		.name=source->name(),
		.uri=source->uri(),
		.images=std::nullopt,
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
		FGL_ASSERT(std::dynamic_pointer_cast<ShuffledTrackCollection>(source) == nullptr, "Cannot create ShuffledTrackCollection with another ShuffledTrackCollection");
		
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

	$<TrackCollection> ShuffledTrackCollection::source() {
		return _source;
	}

	$<const TrackCollection> ShuffledTrackCollection::source() const {
		return std::static_pointer_cast<const TrackCollection>(_source);
	}

	Promise<void> ShuffledTrackCollection::fetchData() {
		auto self = selfAs<ShuffledTrackCollection>();
		return _source->fetchDataIfNeeded().then([=]() {
			self->_name = self->_source->name();
			self->_images = self->_source->images();
		});
	}

	ShuffledTrackCollection::MutatorDelegate* ShuffledTrackCollection::createMutatorDelegate() {
		return this;
	}

	Promise<void> ShuffledTrackCollection::loadItems(Mutator* mutator, size_t index, size_t count) {
		auto self = this->selfAs<ShuffledTrackCollection>();
		size_t endIndex = index+count;
		auto items = fgl::new$<LinkedList<$<ShuffledTrackCollectionItem>>>();
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
					return self->_source->getItem(randomIndex->index).then([=]($<TrackCollectionItem> item) {
						items->pushBack(ShuffledTrackCollectionItem::new$(self, item));
					});
				});
			}
		}
		return promise.then([=]() {
			for(auto randomIndex : chosenIndexes) {
				auto it = self->_remainingIndexes.findEqual(randomIndex);
				if(it != self->_remainingIndexes.end()) {
					self->_remainingIndexes.erase(it);
				}
			}
			auto itemCount = self->_source->itemCount();
			if(itemCount) {
				mutator->resize(itemCount.value());
			}
			mutator->apply(index, *items);
		});
	}



	Json ShuffledTrackCollection::toJson(ToJsonOptions options) const {
		auto json = SpecialTrackCollection<ShuffledTrackCollectionItem>::toJson(options).object_items();
		json.merge(Json::object{
			{ "source", _source->toJson({.tracksOffset=0,.tracksLimit=options.tracksLimit}) }
		});
		return json;
	}
}

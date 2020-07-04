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
	$<ShuffledTrackCollectionItem> ShuffledTrackCollectionItem::new$($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, const Data& data) {
		return fgl::new$<ShuffledTrackCollectionItem>(context, data);
	}
	
	$<ShuffledTrackCollectionItem> ShuffledTrackCollectionItem::new$($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, $<TrackCollectionItem> sourceItem) {
		return fgl::new$<ShuffledTrackCollectionItem>(context, sourceItem);
	}
	
	ShuffledTrackCollectionItem::ShuffledTrackCollectionItem($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, const Data& data)
	: SpecialTrackCollectionItem<ShuffledTrackCollection>(context, data), _sourceItem(data.sourceItem) {
		FGL_ASSERT(std::dynamic_pointer_cast<ShuffledTrackCollectionItem>(data.sourceItem) == nullptr, "Cannot create ShuffledTrackCollectionItem with another ShuffledTrackCollectionItem");
	}
	
	ShuffledTrackCollectionItem::ShuffledTrackCollectionItem($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, $<TrackCollectionItem> sourceItem)
	: SpecialTrackCollectionItem<ShuffledTrackCollection>(context, {.track=sourceItem->track()}), _sourceItem(sourceItem) {
		FGL_ASSERT(std::dynamic_pointer_cast<ShuffledTrackCollectionItem>(sourceItem) == nullptr, "Cannot create ShuffledTrackCollectionItem with another ShuffledTrackCollectionItem");
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

	$<ShuffledTrackCollectionItem> ShuffledTrackCollectionItem::fromJson($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, const Json& json, MediaProviderStash* stash) {
		return fgl::new$<ShuffledTrackCollectionItem>(context, json, stash);
	}

	ShuffledTrackCollectionItem::ShuffledTrackCollectionItem($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, const Json& json, MediaProviderStash* stash)
	: SpecialTrackCollectionItem<ShuffledTrackCollection>(context, json, stash) {
		auto shuffledContext = std::static_pointer_cast<ShuffledTrackCollection>(context);
		auto sourceContext = shuffledContext->source();
		_sourceItem = sourceContext->itemFromJson(json["sourceItem"], stash);
		_track = _sourceItem->track();
	}

	Json ShuffledTrackCollectionItem::toJson() const {
		auto json = SpecialTrackCollectionItem<ShuffledTrackCollection>::toJson().object_items();
		auto sourceIndex = _sourceItem->indexInContext();
		json.merge(Json::object{
			{ "sourceItem", _sourceItem->toJson() },
			{ "sourceIndex", sourceIndex ? Json((double)sourceIndex.value()) : Json() }
		});
		return json;
	}




	$<ShuffledTrackCollection> ShuffledTrackCollection::new$($<TrackCollection> source, ArrayList<$<TrackCollectionItem>> initialItems) {
		auto collection = fgl::new$<ShuffledTrackCollection>(source, initialItems);
		collection->lazyLoadContentIfNeeded();
		return collection;
	}

	ShuffledTrackCollection::ShuffledTrackCollection($<TrackCollection> source, ArrayList<$<TrackCollectionItem>> initialItems)
	: SpecialTrackCollection<ShuffledTrackCollectionItem>(source->mediaProvider(), SpecialTrackCollection<ShuffledTrackCollectionItem>::Data{{
		.partial=source->needsData(),
		.type="shuffled-collection",
		.name=source->name(),
		.uri=source->uri(),
		.images=std::nullopt,
		},
		.versionId=source->versionId(),
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
		
		auto lazyContentLoader = _lazyContentLoader;
		_lazyContentLoader = [=]() {
			lazyContentLoader();
			auto contentLoaderRef = _lazyContentLoader;
			_lazyContentLoader = nullptr;
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
					indexes.pushBack(i);
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
		};
	}

	String ShuffledTrackCollection::versionId() const {
		return _source->versionId();
	}

	$<TrackCollection> ShuffledTrackCollection::source() {
		return _source;
	}

	$<const TrackCollection> ShuffledTrackCollection::source() const {
		return std::static_pointer_cast<const TrackCollection>(_source);
	}

	Promise<void> ShuffledTrackCollection::fetchData() {
		auto self = std::static_pointer_cast<ShuffledTrackCollection>(shared_from_this());
		return _source->fetchDataIfNeeded().then([=]() {
			self->_name = self->_source->name();
			self->_images = self->_source->images();
		});
	}

	ShuffledTrackCollection::MutatorDelegate* ShuffledTrackCollection::createMutatorDelegate() {
		return this;
	}

	Promise<void> ShuffledTrackCollection::loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) {
		auto self = std::static_pointer_cast<ShuffledTrackCollection>(shared_from_this());
		size_t endIndex = index+count;
		auto items = fgl::new$<LinkedList<$<ShuffledTrackCollectionItem>>>();
		auto promise = Promise<void>::resolve();
		auto tmpRemainingIndexes = _remainingIndexes;
		auto chosenIndexes = LinkedList<$<RandomIndex>>();
		for(size_t i=index; i<endIndex; i++) {
			auto existingItem = itemAt(i);
			if(existingItem) {
				if(auto shuffledItem = std::dynamic_pointer_cast<ShuffledTrackCollectionItem>(existingItem)) {
					promise = promise.then([=]() {
						items->pushBack(shuffledItem);
					});
				} else {
					promise = promise.then([=]() {
						items->pushBack(ShuffledTrackCollectionItem::new$(self, existingItem));
					});
				}
			} else {
				if(tmpRemainingIndexes.empty()) {
					break;
				}
				auto randomIndex = tmpRemainingIndexes.extractFront();
				chosenIndexes.pushBack(randomIndex);
				promise = promise.then([=]() {
					return self->_source->getItem(randomIndex->index, options).then([=]($<TrackCollectionItem> item) {
						items->pushBack(ShuffledTrackCollectionItem::new$(self, item));
					});
				});
			}
		}
		return promise.then([=]() {
			mutator->lock([&]() {
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
		});
	}



	Json ShuffledTrackCollection::toJson(const ToJsonOptions& options) const {
		auto json = SpecialTrackCollection<ShuffledTrackCollectionItem>::toJson(options).object_items();
		json.merge(Json::object{
			{ "source", _source->toJson({.tracksOffset=0,.tracksLimit=options.tracksLimit}) }
		});
		return json;
	}
}

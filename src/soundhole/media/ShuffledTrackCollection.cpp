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

	#pragma mark ShuffledTrackCollectionItem::Data

	ShuffledTrackCollectionItem::Data ShuffledTrackCollectionItem::Data::forSourceItem($<TrackCollectionItem> sourceItem) {
		FGL_ASSERT(std::dynamic_pointer_cast<ShuffledTrackCollectionItem>(sourceItem) == nullptr, "Cannot create ShuffledTrackCollectionItem::Data with a ShuffledTrackCollectionItem source item");
		return ShuffledTrackCollectionItem::Data{{
			.track=sourceItem->track()
			},
			.sourceItem=sourceItem
		};
	}



	#pragma mark ShuffledTrackCollectionItem

	$<ShuffledTrackCollectionItem> ShuffledTrackCollectionItem::new$($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, const Data& data) {
		return fgl::new$<ShuffledTrackCollectionItem>(context, data);
	}
	
	ShuffledTrackCollectionItem::ShuffledTrackCollectionItem($<SpecialTrackCollection<ShuffledTrackCollectionItem>> context, const Data& data)
	: SpecialTrackCollectionItem<ShuffledTrackCollection>(context, data), _sourceItem(data.sourceItem) {
		FGL_ASSERT(std::dynamic_pointer_cast<ShuffledTrackCollectionItem>(data.sourceItem) == nullptr, "Cannot create ShuffledTrackCollectionItem with another ShuffledTrackCollectionItem");
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
		.itemCount=source->itemCount().valueOr(initialItems.size()),
		.items=([&]() {
			auto items = std::map<size_t,ShuffledTrackCollectionItem::Data>();
			for(size_t i=0; i<initialItems.size(); i++) {
				auto sourceItem = initialItems[i];
				FGL_ASSERT(std::dynamic_pointer_cast<ShuffledTrackCollectionItem>(sourceItem) == nullptr, "Cannot use ShuffledTrackCollectionItem as sourceItem");
				items.insert_or_assign(i, ShuffledTrackCollectionItem::Data::forSourceItem(sourceItem));
			}
			return items;
		})()
	}), _source(source) {
		FGL_ASSERT(std::dynamic_pointer_cast<ShuffledTrackCollection>(source) == nullptr, "Cannot create ShuffledTrackCollection with another ShuffledTrackCollection");
		
		auto lazyContentLoader = _lazyContentLoader;
		_lazyContentLoader = [=]() {
			if(lazyContentLoader) {
				lazyContentLoader();
			}
			
			source->lockItems([&]() {
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
					_remainingIndexes.pushBack(_source->watchIndex(randomIndex));
					indexes.erase(it);
				}
				
				// TODO subscribe to changes in source
			});
		};
	}

	ShuffledTrackCollection::~ShuffledTrackCollection() {
		for(auto indexMarker : _remainingIndexes) {
			_source->unwatchIndex(indexMarker);
		}
		// TODO unsubscribe from source
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

	size_t ShuffledTrackCollection::getChunkSize() const {
		return 12;
	}

	void ShuffledTrackCollection::filterRemainingIndexes() {
		_remainingIndexes.removeWhere([=](auto& indexMarker) {
			if(indexMarker->state == ItemIndexMarkerState::REMOVED) {
				_source->unwatchIndex(indexMarker);
				return true;
			}
			return false;
		});
	}

	size_t ShuffledTrackCollection::shuffledListSize() const {
		if(tracksAreAsync()) {
			return asyncItemsList()->getMap().size() + _remainingIndexes.size();
		} else if(tracksAreEmpty()) {
			return _remainingIndexes.size();
		} else {
			return _source->itemCount().value_or(0);
		}
	}

	Promise<void> ShuffledTrackCollection::loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) {
		auto self = std::static_pointer_cast<ShuffledTrackCollection>(shared_from_this());
		std::unique_ptr<Promise<void>> promise_ptr;
		_source->lockItems([&]() {
			filterRemainingIndexes();
			size_t endIndex = index+count;
			auto items = fgl::new$<LinkedList<$<Item>>>();
			auto promise = Promise<void>::resolve();
			auto tmpRemainingIndexes = _remainingIndexes;
			auto chosenIndexes = LinkedList<AsyncListIndexMarker>();
			for(size_t i=index; i<endIndex; i++) {
				auto existingItem = std::static_pointer_cast<Item>(itemAt(i));
				if(existingItem) {
					promise = promise.then(nullptr, [=]() {
						items->pushBack(existingItem);
					});
				} else {
					if(tmpRemainingIndexes.empty()) {
						break;
					}
					auto randomIndex = tmpRemainingIndexes.extractFront();
					chosenIndexes.pushBack(randomIndex);
					promise = promise.then(nullptr, [=]() {
						if(randomIndex->state == AsyncListIndexMarkerState::REMOVED) {
							throw std::runtime_error("Random index at "+stringify(randomIndex->index)+" was removed");
						}
						return self->_source->getItem(randomIndex->index, {.trackIndexChanges=true}).then(nullptr, [=]($<TrackCollectionItem> item) {
							if(!item) {
								throw std::runtime_error("Item not found at index "+stringify(randomIndex->index));
							}
							items->pushBack(self->createCollectionItem(Item::Data::forSourceItem(item)));
						});
					});
				}
			}
			promise_ptr = std::make_unique<Promise<void>>(promise.then(nullptr, [=]() {
				mutator->lock([&]() {
					_source->lockItems([&]() {
						filterRemainingIndexes();
						size_t listSize = shuffledListSize();
						for(auto randomIndex : chosenIndexes) {
							auto it = self->_remainingIndexes.findEqual(randomIndex);
							if(it != self->_remainingIndexes.end()) {
								self->_remainingIndexes.erase(it);
							}
						}
						mutator->applyAndResize(index, listSize, *items);
					});
				});
			}));
		});
		return std::move(*promise_ptr);
	}

	Promise<void> ShuffledTrackCollection::insertItems(Mutator* mutator, size_t index, LinkedList<$<Track>> tracks) {
		return Promise<void>::reject(std::runtime_error("Cannot insert items into a ShuffledTrackCollection"));
	}

	Promise<void> ShuffledTrackCollection::appendItems(Mutator* mutator, LinkedList<$<Track>> tracks) {
		return Promise<void>::reject(std::runtime_error("Cannot append items to a ShuffledTrackCollection"));
	}

	Promise<void> ShuffledTrackCollection::removeItems(Mutator* mutator, size_t index, size_t count) {
		return Promise<void>::reject(std::runtime_error("Cannot remove items from a ShuffledTrackCollection"));
	}

	Promise<void> ShuffledTrackCollection::moveItems(Mutator* mutator, size_t index, size_t count, size_t newIndex) {
		return Promise<void>::reject(std::runtime_error("Cannot move items inside a ShuffledTrackCollection"));
	}



	Json ShuffledTrackCollection::toJson(const ToJsonOptions& options) const {
		auto json = SpecialTrackCollection<ShuffledTrackCollectionItem>::toJson(options).object_items();
		json.merge(Json::object{
			{ "source", _source->toJson({
				.itemsStartIndex = options.itemsStartIndex,
				.itemsEndIndex = options.itemsEndIndex
			}) }
		});
		return json;
	}
}

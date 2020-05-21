//
//  TrackCollection.impl.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/24/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

#define TRACKCOLLECTION_CHUNK_SIZE 18

namespace sh {
	template<typename T>
	$<T> TrackCollectionItem::selfAs() {
		return std::static_pointer_cast<T>(weakSelf.lock());
	}

	template<typename T>
	$<const T> TrackCollectionItem::selfAs() const {
		return std::static_pointer_cast<const T>(weakSelf.lock());
	}


	template<typename ItemType>
	SpecialTrackCollection<ItemType>::SpecialTrackCollection($<MediaItem>& ptr, MediaProvider* provider, Data data)
	: TrackCollection(ptr, provider, data), _items(constructItems(data.tracks)), _mutatorDelegate(nullptr), autoDeleteMutatorDelegate(true) {
		//
	}

	template<typename ItemType>
	SpecialTrackCollection<ItemType>::~SpecialTrackCollection() {
		if(_mutatorDelegate != nullptr && autoDeleteMutatorDelegate) {
			delete _mutatorDelegate;
		}
	}



	template<typename ItemType>
	std::variant<std::nullptr_t,typename SpecialTrackCollection<ItemType>::EmptyTracks,LinkedList<$<ItemType>>,$<AsyncList<$<ItemType>>>>
	SpecialTrackCollection<ItemType>::constructItems(Optional<typename Data::Tracks> tracks) {
		auto self = this->template selfAs<SpecialTrackCollection<ItemType>>();
		if(!tracks.has_value()) {
			return std::nullptr_t();
		} else if(tracks->items.size() == 0) {
			return EmptyTracks{
				.total = tracks->total
			};
		} else if(tracks->offset == 0 && tracks->items.size() == tracks->total) {
			return tracks->items.template map<$<ItemType>>([&](auto& data) {
				return ItemType::new$(self, data);
			});
		} else {
			return AsyncList<$<ItemType>>::new$({
				.delegate=this,
				.chunkSize=TRACKCOLLECTION_CHUNK_SIZE,
				.initialItemsOffset=tracks->offset,
				.initialItems=ArrayList<$<ItemType>>(tracks->items.template map<$<ItemType>>([&](auto& data) {
					return ItemType::new$(self, data);
				})),
				.initialSize=tracks->total
			});
		}
	}

	template<typename ItemType>
	Optional<size_t> SpecialTrackCollection<ItemType>::indexOfItem(const TrackCollectionItem* item) const {
		auto castItem = dynamic_cast<const ItemType*>(item);
		if(castItem == nullptr) {
			return std::nullopt;
		}
		return indexOfItem(castItem);
	}

	template<typename ItemType>
	Optional<size_t> SpecialTrackCollection<ItemType>::indexOfItem(const ItemType* item) const {
		if(tracksAreEmpty()) {
			return std::nullopt;
		}
		if(tracksAreAsync()) {
			return asyncItemsList()->indexWhere([&](auto& cmpItem) {
				return (item == cmpItem.get() || cmpItem->matchesItem(item));
			});
		} else {
			size_t index = 0;
			auto& items = itemsList();
			for(auto& cmpItem : items) {
				if(item == cmpItem.get() || cmpItem->matchesItem(item)) {
					return index;
				}
				index++;
			}
			return std::nullopt;
		}
	}

	template<typename ItemType>
	Optional<size_t> SpecialTrackCollection<ItemType>::indexOfItemInstance(const TrackCollectionItem* item) const {
		auto castItem = dynamic_cast<const ItemType*>(item);
		if(castItem == nullptr) {
			return std::nullopt;
		}
		return indexOfItemInstance(castItem);
	}

	template<typename ItemType>
	Optional<size_t> SpecialTrackCollection<ItemType>::indexOfItemInstance(const ItemType* item) const {
		if(tracksAreEmpty()) {
			return std::nullopt;
		}
		if(tracksAreAsync()) {
			return asyncItemsList()->indexWhere([&](auto& cmpItem) {
				return (item == cmpItem.get());
			});
		} else {
			size_t index = 0;
			auto& items = itemsList();
			for(auto& cmpItem : items) {
				if(item == cmpItem.get()) {
					return index;
				}
				index++;
			}
			return std::nullopt;
		}
	}

	template<typename ItemType>
	$<TrackCollectionItem> SpecialTrackCollection<ItemType>::itemAt(size_t index) {
		if(tracksAreEmpty()) {
			return nullptr;
		} else if(tracksAreAsync()) {
			return std::static_pointer_cast<TrackCollectionItem>(asyncItemsList()->itemAt(index).value_or(nullptr));
		} else {
			auto& items = itemsList();
			if(index >= items.size()) {
				return nullptr;
			}
			return std::static_pointer_cast<TrackCollectionItem>(*std::next(items.begin(), index));
		}
	}

	template<typename ItemType>
	$<const TrackCollectionItem> SpecialTrackCollection<ItemType>::itemAt(size_t index) const {
		if(tracksAreEmpty()) {
			return nullptr;
		} else if(tracksAreAsync()) {
			return std::static_pointer_cast<const TrackCollectionItem>(asyncItemsList()->itemAt(index).value_or(nullptr));
		} else {
			auto& items = itemsList();
			if(index >= items.size()) {
				return nullptr;
			}
			return std::static_pointer_cast<const TrackCollectionItem>(*std::next(items.begin(), index));
		}
	}

	template<typename ItemType>
	Promise<$<TrackCollectionItem>> SpecialTrackCollection<ItemType>::getItem(size_t index) {
		if(!tracksAreEmpty() && !tracksAreAsync()) {
			auto& items = itemsList();
			if(index >= items.size()) {
				return Promise<$<TrackCollectionItem>>::resolve(nullptr);
			}
			return Promise<$<TrackCollectionItem>>::resolve(
				std::static_pointer_cast<TrackCollectionItem>(*std::next(items.begin(), index)));
		}
		makeTracksAsync();
		return asyncItemsList()->getItem(index).template map<$<TrackCollectionItem>>(nullptr, [](auto item) {
			return std::static_pointer_cast<TrackCollectionItem>(item.value_or(nullptr));
		});
	}

	template<typename ItemType>
	Promise<LinkedList<$<TrackCollectionItem>>> SpecialTrackCollection<ItemType>::getItems(size_t index, size_t count) {
		if(!tracksAreEmpty() && !tracksAreAsync()) {
			auto& items = itemsList();
			if(index >= items.size()) {
				return Promise<LinkedList<$<TrackCollectionItem>>>::resolve(LinkedList<$<TrackCollectionItem>>());
			}
			auto startIt = std::next(items.begin(), index);
			auto endIt = ((index+count) < items.size()) ? std::next(startIt, count) : items.end();
			LinkedList<$<TrackCollectionItem>> loadedItems;
			for(auto it=startIt; it!=endIt; it++) {
				loadedItems.pushBack(std::static_pointer_cast<TrackCollectionItem>(*it));
			}
			return Promise<LinkedList<$<TrackCollectionItem>>>::resolve(loadedItems);
		}
		makeTracksAsync();
		return this->asyncItemsList()->getItems(index, count).template map<LinkedList<$<TrackCollectionItem>>>(nullptr, [](auto items) {
			return items.template map<$<TrackCollectionItem>>([](auto& item) {
				return std::static_pointer_cast<TrackCollectionItem>(item);
			});
		});
	}

	template<typename ItemType>
	typename TrackCollection::ItemGenerator SpecialTrackCollection<ItemType>::generateItems(size_t startIndex) {
		if(tracksAreEmpty()) {
			makeTracksAsync();
		}
		if(tracksAreAsync()) {
			return asyncItemsList()->generateItems(startIndex).template map<LinkedList<$<TrackCollectionItem>>>([](auto items) {
				return items.template map<$<TrackCollectionItem>>([](auto& item) {
					return std::static_pointer_cast<TrackCollectionItem>(item);
				});
			});
		} else {
			size_t chunkSize = TRACKCOLLECTION_CHUNK_SIZE;
			auto& sourceList = itemsList();
			auto startIt = sourceList.begin();
			size_t i=0;
			while(startIt != sourceList.end() && i < startIndex) {
				startIt++;
				i++;
			}
			auto items = fgl::new$<LinkedList<$<ItemType>>>(startIt, sourceList.end());
			return ItemGenerator([=]() {
				using YieldResult = typename ItemGenerator::YieldResult;
				LinkedList<$<TrackCollectionItem>> genItems;
				while(genItems.size() < chunkSize && items->size() > 0) {
					genItems.pushBack(std::static_pointer_cast<TrackCollectionItem>(items->front()));
					items->popFront();
				}
				if(items->size() == 0) {
					return Promise<YieldResult>::resolve(YieldResult{
						.value=genItems,
						.done=true
					});
				} else {
					return Promise<YieldResult>::resolve(YieldResult{
						.value=genItems,
						.done=false
					});
				}
			});
		}
	}

	template<typename ItemType>
	Optional<size_t> SpecialTrackCollection<ItemType>::itemCount() const {
		if(_items.index() == 1) {
			return std::get<EmptyTracks>(_items).total;
		} else if(_items.index() == 2) {
			return std::get<LinkedList<$<ItemType>>>(_items).size();
		} else if(_items.index() == 3) {
			auto asyncItems = std::get<$<AsyncList<$<ItemType>>>>(_items);
			if(asyncItems->sizeIsKnown()) {
				return asyncItems->size();
			}
			return std::nullopt;
		}
		return std::nullopt;
	}
	
	template<typename ItemType>
	Promise<void> SpecialTrackCollection<ItemType>::loadItems(size_t index, size_t count) {
		if(tracksAreEmpty()) {
			makeTracksAsync();
		}
		if(tracksAreAsync()) {
			return asyncItemsList()->getItems(index,count).toVoid();
		} else {
			return Promise<void>::resolve();
		}
	}

	template<typename ItemType>
	void SpecialTrackCollection<ItemType>::applyData(const Data& data) {
		TrackCollection::applyData(data);
		if(data.tracks) {
			auto tracks = data.tracks.value();
			if(tracks.offset == 0 && tracks.items.size() == tracks.total) {
				if(tracksAreAsync()) {
					asyncItemsList()->mutate([=](auto mutator) {
						auto self = selfAs<SpecialTrackCollection<ItemType>>();
						mutator->applyAndResize(tracks.offset, tracks.total, tracks.items.template map<$<ItemType>>([&](auto& data) {
							return ItemType::new$(self, data);
						}));
					});
				} else {
					// TODO merge the array
					if(itemsList().size() != tracks.total) {
						_items = constructItems(tracks);
					}
				}
			} else {
				makeTracksAsync();
				asyncItemsList()->mutate([=](auto mutator) {
					auto self = selfAs<SpecialTrackCollection<ItemType>>();
					mutator->applyAndResize(tracks.offset, tracks.total, tracks.items.template map<$<ItemType>>([&](auto& data) {
						return ItemType::new$(self, data);
					}));
				});
			}
		}
	}



	template<typename ItemType>
	bool SpecialTrackCollection<ItemType>::areAsyncListItemsEqual(const AsyncList<$<ItemType>>* list, const $<ItemType>& item1, const $<ItemType>& item2) const {
		return item1->matchesItem(item2.get());
	}

	template<typename ItemType>
	Promise<void> SpecialTrackCollection<ItemType>::loadAsyncListItems(typename AsyncList<$<ItemType>>::Mutator* mutator, size_t index, size_t count) {
		auto delegate = mutatorDelegate();
		return delegate->loadItems(mutator, index, count);
	}

	template<typename ItemType>
	typename SpecialTrackCollection<ItemType>::MutatorDelegate* SpecialTrackCollection<ItemType>::mutatorDelegate() {
		if(_mutatorDelegate == nullptr) {
			_mutatorDelegate = createMutatorDelegate();
			if(_mutatorDelegate == nullptr) {
				throw std::logic_error("createMutatorDelegate returned null");
			}
			if(dynamic_cast<MutatorDelegate*>(this) == _mutatorDelegate) {
				autoDeleteMutatorDelegate = false;
			}
		}
		return _mutatorDelegate;
	}



	template<typename ItemType>
	bool SpecialTrackCollection<ItemType>::tracksAreEmpty() const {
		return std::get_if<std::nullptr_t>(&_items) != nullptr || std::get_if<EmptyTracks>(&_items) != nullptr;
	}

	template<typename ItemType>
	bool SpecialTrackCollection<ItemType>::tracksAreAsync() const {
		return std::get_if<$<AsyncList<$<ItemType>>>>(&_items) != nullptr;
	}

	template<typename ItemType>
	LinkedList<$<ItemType>>& SpecialTrackCollection<ItemType>::itemsList() {
		return std::get<LinkedList<$<ItemType>>>(_items);
	}

	template<typename ItemType>
	const LinkedList<$<ItemType>>& SpecialTrackCollection<ItemType>::itemsList() const {
		return std::get<LinkedList<$<ItemType>>>(_items);
	}

	template<typename ItemType>
	$<AsyncList<$<ItemType>>> SpecialTrackCollection<ItemType>::asyncItemsList() {
		return std::get<$<AsyncList<$<ItemType>>>>(_items);
	}

	template<typename ItemType>
	$<const AsyncList<$<ItemType>>> SpecialTrackCollection<ItemType>::asyncItemsList() const {
		return std::static_pointer_cast<const AsyncList<$<ItemType>>>(std::get<$<AsyncList<$<ItemType>>>>(_items));
	}

	template<typename ItemType>
	void SpecialTrackCollection<ItemType>::makeTracksAsync() {
		if(tracksAreAsync()) {
			return;
		} else if(tracksAreEmpty()) {
			if(_items.index() == 0) {
				_items = AsyncList<$<ItemType>>::new$({
					.delegate=this,
					.chunkSize=TRACKCOLLECTION_CHUNK_SIZE
				});
			} else {
				auto& tracks = std::get<EmptyTracks>(_items);
				_items = AsyncList<$<ItemType>>::new$({
					.delegate=this,
					.chunkSize=TRACKCOLLECTION_CHUNK_SIZE,
					.initialSize=tracks.total
				});
			}
		} else {
			auto& tracks = itemsList();
			_items = AsyncList<$<ItemType>>::new$({
				.delegate=this,
				.chunkSize=TRACKCOLLECTION_CHUNK_SIZE,
				.initialItemsOffset=0,
				.initialItems=tracks,
				.initialSize=tracks.size()
			});
		}
	}

	template<typename ItemType>
	typename SpecialTrackCollection<ItemType>::Data SpecialTrackCollection<ItemType>::toData(DataOptions options) const {
		return SpecialTrackCollection<ItemType>::Data{
			TrackCollection::toData(),
			.tracks=([&]() -> Optional<typename Data::Tracks> {
				if(auto emptyTracks = std::get_if<EmptyTracks>(&_items)) {
					return typename Data::Tracks{
						.total=emptyTracks->total,
						.offset=0,
						.items={}
					};
				} else if(auto items = std::get_if<LinkedList<$<ItemType>>>(&_items)) {
					return typename Data::Tracks{
						.total=items->size(),
						.offset=0,
						.items=items->template map<typename ItemType::Data>([&]($<ItemType> item) -> typename ItemType::Data {
							return item->toData();
						})
					};
				} else if(auto asyncItemsPtr = std::get_if<$<AsyncList<$<ItemType>>>>(&_items)) {
					auto asyncItems = *asyncItemsPtr;
					return typename Data::Tracks{
						.total=asyncItems->size(),
						.offset=0,
						.items=asyncItems->getLoadedItems({
							.startIndex=options.tracksOffset,
							.limit=options.tracksLimit
						}).template map<typename ItemType::Data>([&]($<ItemType> item) -> typename ItemType::Data {
							return item->toData();
						})
					};
				}
				return std::nullopt;
			})()
		};
	}




	template<typename ItemType>
	SpecialTrackCollection<ItemType>::SpecialTrackCollection($<MediaItem>& ptr, Json json, MediaProviderStash* stash)
	: TrackCollection(ptr, json, stash), _items(std::nullptr_t()), _mutatorDelegate(nullptr), autoDeleteMutatorDelegate(true) {
		std::map<size_t,$<ItemType>> items;
		auto itemCountJson = json["itemCount"];
		size_t itemCount = (size_t)itemCountJson.number_value();
		for(auto& pair : json["items"].object_items()) {
			size_t index;
			try {
				index = (size_t)std::stod(pair.first);
			} catch(std::exception&) {
				continue;
			}
			if(itemCount <= index) {
				itemCount = index+1;
			}
			items[index] = std::static_pointer_cast<ItemType>(this->itemFromJson(pair.second, stash));
		}
		bool needsAsync = false;
		if(items.size() == itemCount) {
			size_t i=0;
			for(auto& pair : items) {
				if(pair.first != i) {
					needsAsync = true;
					break;
				}
				i++;
			}
		} else {
			needsAsync = true;
		}
		if(itemCountJson.is_null() && items.size() == 0) {
			_items = std::nullptr_t();
		} else if(needsAsync) {
			_items = AsyncList<$<ItemType>>::new$({
				.delegate=this,
				.chunkSize=TRACKCOLLECTION_CHUNK_SIZE,
				.initialItemsMap=items,
				.initialSize=itemCount
			});
		} else {
			LinkedList<$<ItemType>> itemList;
			for(auto& pair : items) {
				itemList.pushBack(pair.second);
			}
			_items = itemList;
		}
	}

	template<typename ItemType>
	$<TrackCollectionItem> SpecialTrackCollection<ItemType>::itemFromJson(Json json, MediaProviderStash* stash) {
		auto self = selfAs<SpecialTrackCollection<ItemType>>();
		return ItemType::fromJson(self, json, stash);
	}

	template<typename ItemType>
	Json SpecialTrackCollection<ItemType>::toJson(ToJsonOptions options) const {
		auto json = TrackCollection::toJson(options).object_items();
		if(auto items = std::get_if<LinkedList<$<ItemType>>>(&_items)) {
			json.merge(Json::object{
				{ "items", items->template map<Json>([&](auto& item) {
					return item->toJson();
				}) }
			});
		} else if(auto asyncItemsPtr = std::get_if<$<AsyncList<$<ItemType>>>>(&_items)) {
			auto asyncItems = *asyncItemsPtr;
			size_t endIndex = options.tracksOffset + options.tracksLimit;
			if(endIndex < options.tracksLimit || endIndex < options.tracksOffset) {
				endIndex = -1;
			}
			Json::object itemsJson;
			asyncItems->forValidInRange(options.tracksOffset, endIndex, [&](auto& item, size_t index) {
				itemsJson[std::to_string(index)] = item->toJson();
			});
			json.merge(Json::object{
				{ "items", itemsJson }
			});
		}
		return json;
	}



	template<typename Context>
	w$<Context> SpecialTrackCollectionItem<Context>::context() {
		return std::static_pointer_cast<Context>(_context.lock());
	}

	template<typename Context>
	w$<const Context> SpecialTrackCollectionItem<Context>::context() const {
		return std::static_pointer_cast<const Context>(_context.lock());
	}
}

//
//  TrackCollection.impl.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/24/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {

	#pragma mark SpecialTrackCollection::Data

	template<typename ItemType>
	typename SpecialTrackCollection<ItemType>::Data SpecialTrackCollection<ItemType>::Data::fromJson(const Json& json, MediaProviderStash* stash) {
		auto collectionData = TrackCollection::Data::fromJson(json,stash);
		auto itemCountJson = json["itemCount"];
		Optional<size_t> itemCount = itemCountJson.is_number() ? maybe((size_t)itemCountJson.number_value()) : std::nullopt;
		std::map<size_t,typename ItemType::Data> items;
		auto itemsJson = json["items"];
		if(itemsJson.is_object()) {
			for(auto& pair : itemsJson.object_items()) {
				size_t index;
				try {
					index = (size_t)std::stod(pair.first);
				} catch(std::exception&) {
					continue;
				}
				if(pair.second.is_null()) {
					continue;
				}
				items[index] = ItemType::Data::fromJson(pair.second, stash);
			}
		}
		else if(itemsJson.is_array()) {
			size_t i = 0;
			for(auto& itemJson : itemsJson.array_items()) {
				if(itemJson.is_null()) {
					i++;
					continue;
				}
				items[i] = ItemType::Data::fromJson(itemJson, stash);
				i++;
			}
		}
		return SpecialTrackCollection<ItemType>::Data{
			collectionData,
			.versionId = json["versionId"].string_value(),
			.itemCount = itemCount,
			.items = items
		};
	}



	#pragma mark SpecialTrackCollection

	template<typename ItemType>
	SpecialTrackCollection<ItemType>::SpecialTrackCollection(MediaProvider* provider, const Data& data)
	: TrackCollection(provider, data), _versionId(data.versionId), _items(nullptr), _mutatorDelegate(nullptr), autoDeleteMutatorDelegate(true) {
		auto items = data.items;
		auto itemCount = data.itemCount;
		_lazyContentLoader = [=]() {
			_items = constructItems(items, itemCount);
		};
	}

	template<typename ItemType>
	SpecialTrackCollection<ItemType>::~SpecialTrackCollection() {
		if(_mutatorDelegate != nullptr && autoDeleteMutatorDelegate) {
			delete _mutatorDelegate;
		}
		for(auto subscriber : _subscribers) {
			if(auto castSubscriber = dynamic_cast<AutoDeletedSubscriber*>(subscriber)) {
				delete castSubscriber;
			}
		}
	}



	template<typename ItemType>
	void SpecialTrackCollection<ItemType>::lazyLoadContentIfNeeded() const {
		if(_lazyContentLoader) {
			_lazyContentLoader();
			_lazyContentLoader = nullptr;
		}
	}



	template<typename ItemType>
	typename SpecialTrackCollection<ItemType>::ItemsListVariant
	SpecialTrackCollection<ItemType>::constructItems(std::map<size_t,typename ItemType::Data> itemDatas, Optional<size_t> itemCount) {
		if(!shared_from_this()) {
			throw std::runtime_error("cannot call constructItems before constructor has finished");
		}
		if(itemDatas.size() == 0) {
			if(!itemCount) {
				return std::nullptr_t();
			} else {
				return EmptyTracks{
					.total = itemCount.value()
				};
			}
		} else {
			if(itemDatas.begin()->first == 0 && itemCount && itemCount == itemDatas.size()
			   && itemDatas.size() == ((std::prev(itemDatas.end(),1)->first + 1) - itemDatas.begin()->first)) {
				auto items = LinkedList<$<ItemType>>();
				for(auto& pair : itemDatas) {
					items.pushBack(this->createCollectionItem(pair.second));
				}
				return items;
			} else {
				auto items = std::map<size_t,$<ItemType>>();
				for(auto& pair : itemDatas) {
					items.insert_or_assign(pair.first, this->createCollectionItem(pair.second));
				}
				return AsyncList::new$({
					.delegate=this,
					.initialItems=items,
					.initialSize=itemCount
				});
			}
		}
	}

	template<typename ItemType>
	String SpecialTrackCollection<ItemType>::versionId() const {
		return _versionId;
	}

	template<typename ItemType>
	$<ItemType> SpecialTrackCollection<ItemType>::createCollectionItem(const typename ItemType::Data& data) {
		if(!shared_from_this()) {
			throw std::runtime_error("cannot call createCollectionItem before constructor has finished");
		}
		return ItemType::new$(std::static_pointer_cast<SpecialTrackCollection<ItemType>>(shared_from_this()), data);
	}

	template<typename ItemType>
	$<TrackCollectionItem> SpecialTrackCollection<ItemType>::createCollectionItem(const Json& json, MediaProviderStash* stash) {
		auto self = std::static_pointer_cast<SpecialTrackCollection<ItemType>>(shared_from_this());
		if(!self) {
			throw std::runtime_error("cannot call createCollectionItem before constructor has finished");
		}
		if constexpr(has_staticfunc_fromJson<typename ItemType::Data, typename ItemType::Data,
			$<SpecialTrackCollection<ItemType>>, const Json&, MediaProviderStash*>::value) {
			return this->createCollectionItem(ItemType::Data::fromJson(self,json,stash));
		} else if constexpr(has_staticfunc_fromJson<typename ItemType::Data, typename ItemType::Data,
			const Json&, MediaProviderStash*>::value) {
			return this->createCollectionItem(ItemType::Data::fromJson(json,stash));
		} else {
			throw std::runtime_error("Cannot convert json to collection item type "+stringify_type<ItemType>());
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
		lazyLoadContentIfNeeded();
		if(tracksAreEmpty()) {
			return std::nullopt;
		}
		if(tracksAreAsync()) {
			return asyncItemsList()->indexWhere([&](auto& cmpItem) {
				return (item == cmpItem.get() || cmpItem->matchesItem(item));
			}, {
				.onlyValidItems = false
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
		lazyLoadContentIfNeeded();
		if(tracksAreEmpty()) {
			return std::nullopt;
		}
		if(tracksAreAsync()) {
			return asyncItemsList()->indexWhere([&](auto& cmpItem) {
				return (item == cmpItem.get());
			}, {
				.onlyValidItems = false
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
		lazyLoadContentIfNeeded();
		if(tracksAreEmpty()) {
			return nullptr;
		} else if(tracksAreAsync()) {
			return std::static_pointer_cast<TrackCollectionItem>(asyncItemsList()->itemAt(index, {
				.onlyValidItems = false
			}).value_or(nullptr));
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
		lazyLoadContentIfNeeded();
		if(tracksAreEmpty()) {
			return nullptr;
		} else if(tracksAreAsync()) {
			return std::static_pointer_cast<const TrackCollectionItem>(asyncItemsList()->itemAt(index, {
				.onlyValidItems = false
			}).value_or(nullptr));
		} else {
			auto& items = itemsList();
			if(index >= items.size()) {
				return nullptr;
			}
			return std::static_pointer_cast<const TrackCollectionItem>(*std::next(items.begin(), index));
		}
	}

	template<typename ItemType>
	Promise<$<TrackCollectionItem>> SpecialTrackCollection<ItemType>::getItem(size_t index, LoadItemOptions options) {
		lazyLoadContentIfNeeded();
		if(!tracksAreEmpty() && !tracksAreAsync()) {
			auto& items = itemsList();
			if(index >= items.size()) {
				return Promise<$<TrackCollectionItem>>::resolve(nullptr);
			}
			return Promise<$<TrackCollectionItem>>::resolve(
				std::static_pointer_cast<TrackCollectionItem>(*std::next(items.begin(), index)));
		}
		makeTracksAsync();
		return asyncItemsList()->getItem(index, {
			.forceReload = options.forceReload,
			.trackIndexChanges = options.trackIndexChanges,
			.loadOptions = options.toDict()
		}).template map<$<TrackCollectionItem>>(nullptr, [](auto item) {
			return std::static_pointer_cast<TrackCollectionItem>(item.value_or(nullptr));
		});
	}

	template<typename ItemType>
	Promise<LinkedList<$<TrackCollectionItem>>> SpecialTrackCollection<ItemType>::getItems(size_t index, size_t count, LoadItemOptions options) {
		lazyLoadContentIfNeeded();
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
		return this->asyncItemsList()->getItems(index, count, {
			.forceReload = options.forceReload,
			.trackIndexChanges = options.trackIndexChanges,
			.loadOptions = options.toDict()
		}).template map<LinkedList<$<TrackCollectionItem>>>(nullptr, [](auto items) {
			return items.template map<$<TrackCollectionItem>>([](auto& item) {
				return std::static_pointer_cast<TrackCollectionItem>(item);
			});
		});
	}

	template<typename ItemType>
	typename TrackCollection::ItemGenerator SpecialTrackCollection<ItemType>::generateItems(size_t startIndex, LoadItemOptions options) {
		lazyLoadContentIfNeeded();
		if(tracksAreEmpty()) {
			makeTracksAsync();
		}
		if(tracksAreAsync()) {
			return asyncItemsList()->generateItems(startIndex, {
				.forceReload = options.forceReload,
				.trackIndexChanges = options.trackIndexChanges,
				.loadOptions = options.toDict()
			}).template map<LinkedList<$<TrackCollectionItem>>>([](auto items) {
				return items.template map<$<TrackCollectionItem>>([](auto& item) {
					return std::static_pointer_cast<TrackCollectionItem>(item);
				});
			});
		} else {
			size_t chunkSize = 50;
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
		lazyLoadContentIfNeeded();
		if(_items.index() == 1) {
			return std::get<EmptyTracks>(_items).total;
		} else if(_items.index() == 2) {
			return std::get<LinkedList<$<ItemType>>>(_items).size();
		} else if(_items.index() == 3) {
			return asyncItemsList()->size();
		}
		return std::nullopt;
	}

	template<typename ItemType>
	size_t SpecialTrackCollection<ItemType>::itemCapacity() const {
		lazyLoadContentIfNeeded();
		if(_items.index() == 1) {
			return std::get<EmptyTracks>(_items).total;
		} else if(_items.index() == 2) {
			return std::get<LinkedList<$<ItemType>>>(_items).size();
		} else if(_items.index() == 3) {
			return asyncItemsList()->capacity();
		}
		return 0;
	}

	template<typename ItemType>
	Promise<void> SpecialTrackCollection<ItemType>::loadItems(size_t index, size_t count, LoadItemOptions options) {
		lazyLoadContentIfNeeded();
		if(tracksAreEmpty()) {
			makeTracksAsync();
		}
		if(tracksAreAsync()) {
			return asyncItemsList()->loadItems(index,count, {
				.forceReload = options.forceReload,
				.trackIndexChanges = options.trackIndexChanges,
				.loadOptions = options.toDict()
			});
		} else {
			return Promise<void>::resolve();
		}
	}

	template<typename ItemType>
	void SpecialTrackCollection<ItemType>::forEach(Function<void($<TrackCollectionItem>,size_t)> executor) {
		lazyLoadContentIfNeeded();
		if(tracksAreEmpty()) {
			return;
		}
		if(tracksAreAsync()) {
			auto asyncItems = asyncItemsList();
			asyncItems->forEach(executor);
		} else {
			size_t i=0;
			for(auto& item : itemsList()) {
				executor(item, i);
				i++;
			}
		}
	}

	template<typename ItemType>
	void SpecialTrackCollection<ItemType>::forEach(Function<void($<const TrackCollectionItem>,size_t)> executor) const {
		lazyLoadContentIfNeeded();
		if(tracksAreEmpty()) {
			return;
		}
		if(tracksAreAsync()) {
			auto asyncItems = asyncItemsList();
			asyncItems->forEach(executor);
		} else {
			size_t i=0;
			for(auto& item : itemsList()) {
				executor(item, i);
				i++;
			}
		}
	}

	template<typename ItemType>
	void SpecialTrackCollection<ItemType>::forEachInRange(size_t startIndex, size_t endIndex, Function<void($<TrackCollectionItem>,size_t)> executor) {
		lazyLoadContentIfNeeded();
		if(tracksAreEmpty()) {
			return;
		}
		if(tracksAreAsync()) {
			auto asyncItems = asyncItemsList();
			asyncItems->forEachInRange(startIndex, endIndex, executor);
		} else {
			size_t i=0;
			for(auto& item : itemsList()) {
				if(i < startIndex) {
					i++;
					continue;
				}
				if(i >= endIndex) {
					break;
				}
				executor(item, i);
				i++;
			}
		}
	}

	template<typename ItemType>
	void SpecialTrackCollection<ItemType>::forEachInRange(size_t startIndex, size_t endIndex, Function<void($<const TrackCollectionItem>,size_t)> executor) const {
		lazyLoadContentIfNeeded();
		if(tracksAreEmpty()) {
			return;
		}
		if(tracksAreAsync()) {
			auto asyncItems = asyncItemsList();
			asyncItems->forEachInRange(startIndex, endIndex, executor);
		} else {
			size_t i=0;
			for(auto& item : itemsList()) {
				if(i < startIndex) {
					i++;
					continue;
				}
				if(i >= endIndex) {
					break;
				}
				executor(item, i);
				i++;
			}
		}
	}

	template<typename ItemType>
	void SpecialTrackCollection<ItemType>::invalidateItems(size_t startIndex, size_t endIndex) {
		lazyLoadContentIfNeeded();
		if(tracksAreEmpty()) {
			return;
		}
		if(!tracksAreAsync()) {
			makeTracksAsync();
		}
		auto asyncItems = asyncItemsList();
		asyncItems->invalidateItems(startIndex, endIndex, true);
	}

	template<typename ItemType>
	void SpecialTrackCollection<ItemType>::invalidateAllItems() {
		lazyLoadContentIfNeeded();
		if(tracksAreEmpty()) {
			return;
		}
		if(!tracksAreAsync()) {
			makeTracksAsync();
		}
		auto asyncItems = asyncItemsList();
		asyncItems->invalidateAllItems(true);
	}

	template<typename ItemType>
	void SpecialTrackCollection<ItemType>::resetItems() {
		std::unique_ptr<Promise<void>> promise;
		Optional<size_t> listSize = itemCount();
		if(tracksAreAsync()) {
			auto asyncItems = asyncItemsList();
			asyncItems->resetItems();
			asyncItems->destroy();
		}
		if(listSize.has_value()) {
			_items = EmptyTracks{
				.total = listSize.value()
			};
		} else {
			_items = nullptr;
		}
	}

	template<typename ItemType>
	TrackCollection::ItemIndexMarker SpecialTrackCollection<ItemType>::watchIndex(size_t index) {
		if(!tracksAreAsync()) {
			makeTracksAsync();
		}
		return asyncItemsList()->watchIndex(index);
	}

	template<typename ItemType>
	TrackCollection::ItemIndexMarker SpecialTrackCollection<ItemType>::watchRemovedIndex(size_t index) {
		if(!tracksAreAsync()) {
			makeTracksAsync();
		}
		return asyncItemsList()->watchRemovedIndex(index);
	}

	template<typename ItemType>
	void SpecialTrackCollection<ItemType>::unwatchIndex(ItemIndexMarker indexMarker) {
		if(tracksAreAsync()) {
			asyncItemsList()->unwatchIndex(indexMarker);
		}
	}

	template<typename ItemType>
	void SpecialTrackCollection<ItemType>::subscribe(Subscriber* subscriber) {
		_subscribers.pushBack(subscriber);
	}

	template<typename ItemType>
	void SpecialTrackCollection<ItemType>::unsubscribe(Subscriber* subscriber) {
		bool removed = _subscribers.removeLastEqual(subscriber);
		if(removed) {
			if(auto castSubscriber = dynamic_cast<AutoDeletedSubscriber*>(subscriber)) {
				delete castSubscriber;
			}
		}
	}

	template<typename ItemType>
	LinkedList<TrackCollection::Subscriber*> SpecialTrackCollection<ItemType>::subscribers() const {
		return _subscribers;
	}

	template<typename ItemType>
	void SpecialTrackCollection<ItemType>::lockItems(Function<void()> work) {
		if(!tracksAreAsync()) {
			makeTracksAsync();
		}
		asyncItemsList()->lock([&](auto mutator) {
			work();
		});
	}

	template<typename ItemType>
	void SpecialTrackCollection<ItemType>::applyData(const Data& data) {
		lazyLoadContentIfNeeded();
		TrackCollection::applyData(data);
		if(data.items.size() > 0 || (data.itemCount && data.itemCount != itemCount())) {
			makeTracksAsync();
			// apply tracks
			asyncItemsList()->lock([&](auto mutator) {
				mutator->lock([&]() {
					auto items = data.items.mapValues([&](auto index, auto& itemData) -> $<ItemType> {
						return this->createCollectionItem(itemData);
					});
					if(data.itemCount) {
						mutator->applyAndResize(data.itemCount.value(), items);
					} else {
						mutator->apply(items);
					}
				});
			});
		}
	}



	template<typename ItemType>
	size_t SpecialTrackCollection<ItemType>::getAsyncListChunkSize(const AsyncList* list) const {
		auto nonConstThis = const_cast<SpecialTrackCollection<ItemType>*>(this);
		auto delegate = nonConstThis->mutatorDelegate();
		return delegate->getChunkSize();
	}

	template<typename ItemType>
	bool SpecialTrackCollection<ItemType>::areAsyncListItemsEqual(const AsyncList* list, const $<ItemType>& item1, const $<ItemType>& item2) const {
		return item1->matchesItem(item2.get());
	}

	template<typename ItemType>
	void SpecialTrackCollection<ItemType>::mergeAsyncListItem(const AsyncList* list, $<ItemType>& overwritingItem, $<ItemType>& existingItem) {
		auto newItem = overwritingItem;
		overwritingItem = existingItem;
		overwritingItem->merge(newItem.get());
	}

	template<typename ItemType>
	Promise<void> SpecialTrackCollection<ItemType>::loadAsyncListItems(typename AsyncList::Mutator* mutator, size_t index, size_t count, std::map<String,Any> options) {
		auto delegate = mutatorDelegate();
		return delegate->loadItems(mutator, index, count, LoadItemOptions::fromDict(options));
	}

	template<typename ItemType>
	Promise<void> SpecialTrackCollection<ItemType>::insertAsyncListItems(typename AsyncList::Mutator* mutator, size_t index, LinkedList<$<Track>> tracks) {
		auto delegate = mutatorDelegate();
		return delegate->insertItems(mutator, index, tracks);
	}

	template<typename ItemType>
	Promise<void> SpecialTrackCollection<ItemType>::appendAsyncListItems(typename AsyncList::Mutator* mutator, LinkedList<$<Track>> tracks) {
		auto delegate = mutatorDelegate();
		return delegate->appendItems(mutator, tracks);
	}

	template<typename ItemType>
	Promise<void> SpecialTrackCollection<ItemType>::removeAsyncListItems(typename AsyncList::Mutator* mutator, size_t index, size_t count) {
		auto delegate = mutatorDelegate();
		return delegate->removeItems(mutator, index, count);
	}

	template<typename ItemType>
	Promise<void> SpecialTrackCollection<ItemType>::moveAsyncListItems(typename AsyncList::Mutator* mutator, size_t index, size_t count, size_t newIndex) {
		auto delegate = mutatorDelegate();
		return delegate->moveItems(mutator, index, count, newIndex);
	}

	template<typename ItemType>
	void SpecialTrackCollection<ItemType>::onAsyncListMutations(const AsyncList* list, AsyncListChange change) {
		// pass on mutations to subscribers
		auto collection = std::static_pointer_cast<TrackCollection>(shared_from_this());
		auto subscribers = _subscribers;
		for(auto subscriber : subscribers) {
			subscriber->onTrackCollectionMutations(collection, change);
		}
	}

	template<typename ItemType>
	typename SpecialTrackCollection<ItemType>::MutatorDelegate* SpecialTrackCollection<ItemType>::createMutatorDelegate() {
		return nullptr;
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
		return std::get_if<$<AsyncList>>(&_items) != nullptr;
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
	$<typename SpecialTrackCollection<ItemType>::AsyncList> SpecialTrackCollection<ItemType>::asyncItemsList() {
		return std::get<$<AsyncList>>(_items);
	}

	template<typename ItemType>
	$<const typename SpecialTrackCollection<ItemType>::AsyncList> SpecialTrackCollection<ItemType>::asyncItemsList() const {
		return std::static_pointer_cast<const AsyncList>(std::get<$<AsyncList>>(_items));
	}

	template<typename ItemType>
	void SpecialTrackCollection<ItemType>::makeTracksAsync() {
		lazyLoadContentIfNeeded();
		if(tracksAreAsync()) {
			return;
		} else if(tracksAreEmpty()) {
			if(_items.index() == 0) {
				_items = AsyncList::new$({
					.delegate=this,
				});
			} else {
				auto& tracks = std::get<EmptyTracks>(_items);
				_items = AsyncList::new$({
					.delegate=this,
					.initialSize=tracks.total
				});
			}
		} else {
			auto& tracks = itemsList();
			std::map<size_t,$<ItemType>> items;
			for(auto& track : tracks) {
				size_t index = items.size();
				items.insert_or_assign(index, track);
			}
			_items = AsyncList::new$({
				.delegate=this,
				.initialItems=items,
				.initialSize=items.size()
			});
		}
	}


	template<typename ItemType>
	typename SpecialTrackCollection<ItemType>::Data SpecialTrackCollection<ItemType>::toData(const DataOptions& options) const {
		using ItemDataMap = std::map<size_t,typename ItemType::Data>;
		lazyLoadContentIfNeeded();
		return SpecialTrackCollection<ItemType>::Data{
			TrackCollection::toData(),
			.itemCount = itemCount(),
			.items = ([&]() -> ItemDataMap {
				if(auto emptyTracks = std::get_if<EmptyTracks>(&_items)) {
					return ItemDataMap();
				} else if(auto itemsList = std::get_if<LinkedList<$<ItemType>>>(&_items)) {
					ItemDataMap items;
					size_t index = 0;
					for(auto& item : *itemsList) {
						if(items.size() >= options.itemsLimit) {
							break;
						}
						if(index >= options.itemsEndIndex) {
							break;
						} else if(index >= options.itemsStartIndex) {
							items.insert_or_assign(index, item->toData());
						}
						index++;
					}
					return items;
				} else if(auto asyncItemsPtr = std::get_if<$<AsyncList>>(&_items)) {
					auto asyncItems = *asyncItemsPtr;
					auto& itemsMap = asyncItems->getMap();
					ItemDataMap items;
					for(auto it = itemsMap.lower_bound(options.itemsStartIndex),
						end = itemsMap.lower_bound(options.itemsEndIndex);
						it != end;
						it++) {
						if(items.size() >= options.itemsLimit) {
							break;
						}
						auto& itemNode = it->second;
						if(itemNode.valid) {
							items.insert_or_assign(it->first, itemNode.item->toData());
						}
					}
					return items;
				}
				return ItemDataMap();
			})()
		};
	}


	template<typename ItemType>
	Json SpecialTrackCollection<ItemType>::toJson(const ToJsonOptions& options) const {
		lazyLoadContentIfNeeded();
		auto json = TrackCollection::toJson(options).object_items();
		if(auto items = std::get_if<LinkedList<$<ItemType>>>(&_items)) {
			json.merge(Json::object{
				{ "items", items->template map<Json>([&](auto& item) {
					return item->toJson();
				}) }
			});
		} else if(auto asyncItemsPtr = std::get_if<$<AsyncList>>(&_items)) {
			auto asyncItems = *asyncItemsPtr;
			Json::object itemsJson;
			asyncItems->forEachInRange(options.itemsStartIndex, options.itemsEndIndex, [&](auto& item, size_t index) {
				if(itemsJson.size() < options.itemsLimit) {
					itemsJson[std::to_string(index)] = item->toJson();
				}
			});
			json.merge(Json::object{
				{ "items", itemsJson }
			});
		}
		return json;
	}



	#pragma mark SpecialTrackCollectionItem

	template<typename Context>
	w$<Context> SpecialTrackCollectionItem<Context>::context() {
		return std::static_pointer_cast<Context>(_context.lock());
	}

	template<typename Context>
	w$<const Context> SpecialTrackCollectionItem<Context>::context() const {
		return std::static_pointer_cast<const Context>(_context.lock());
	}
}

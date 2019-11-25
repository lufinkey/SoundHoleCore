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
	template<typename ItemType>
	SpecialTrackCollection<ItemType>::SpecialTrackCollection(MediaProvider* provider, Data data)
	: TrackCollection(provider, data), _items(constructItems(data.tracks)) {
		//
	}

	template<typename ItemType>
	SpecialTrackCollection<ItemType>::~SpecialTrackCollection() {
		if(_items.index() == 3) {
			delete std::get<AsyncList<$<ItemType>>*>(_items);
		}
	}

	

	template<typename ItemType>
	std::variant<std::nullptr_t,typename SpecialTrackCollection<ItemType>::EmptyTracks,LinkedList<$<ItemType>>,AsyncList<$<ItemType>>*>
	SpecialTrackCollection<ItemType>::constructItems(Optional<typename Data::Tracks> tracks) {
		if(!tracks.has_value()) {
			return std::nullptr_t();
		} else if(tracks->items.size() == 0) {
			return EmptyTracks{
				.total = tracks->total
			};
		} else if(tracks->offset == 0 && tracks->items.size() == tracks->total) {
			return tracks->items.template map<$<ItemType>>([&](auto& data) {
				return ItemType::new$(self(), std::move(data));
			});
		} else {
			return new AsyncList<$<ItemType>>({
				.delegate=this,
				.chunkSize=24,
				.initialItemsOffset=tracks->offset,
				.initialItems=ArrayList<$<ItemType>>(tracks->items.template map<$<ItemType>>([&](auto& data) {
					return ItemType::new$(self(), std::move(data));
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
			return asyncItemsList().indexWhere([&](auto& cmpItem) {
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
	$<TrackCollectionItem> SpecialTrackCollection<ItemType>::itemAt(size_t index) {
		if(tracksAreEmpty()) {
			return nullptr;
		} else if(tracksAreAsync()) {
			return std::static_pointer_cast<TrackCollectionItem>(asyncItemsList().itemAt(index).value_or(nullptr));
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
			return std::static_pointer_cast<const TrackCollectionItem>(asyncItemsList().itemAt(index).value_or(nullptr));
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
		return asyncItemsList().getItem(index).template map<$<TrackCollectionItem>>(nullptr, [](auto item) {
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
		return this->asyncItemsList().getItems(index, count).template map<LinkedList<$<TrackCollectionItem>>>(nullptr, [](auto items) {
			return items.template map<$<TrackCollectionItem>>([](auto& item) {
				return std::static_pointer_cast<TrackCollectionItem>(item);
			});
		});
	}

	template<typename ItemType>
	Generator<LinkedList<$<TrackCollectionItem>>,void> SpecialTrackCollection<ItemType>::generateItems(size_t startIndex) {
		if(tracksAreEmpty()) {
			makeTracksAsync();
		}
		if(tracksAreAsync()) {
			return asyncItemsList().generateItems(startIndex).template map<LinkedList<$<TrackCollectionItem>>>([](auto items) {
				return items.template map<$<TrackCollectionItem>>([](auto& item) {
					return std::static_pointer_cast<TrackCollectionItem>(item);
				});
			});
		} else {
			size_t chunkSize = 24;
			auto index = new$<size_t>(startIndex);
			auto items = new$<LinkedList<$<ItemType>>>(itemsList());
			return Generator<LinkedList<$<TrackCollectionItem>>,void>([=]() {
				using YieldResult = typename Generator<LinkedList<$<TrackCollectionItem>>,void>::YieldResult;
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
			auto asyncItems = std::get<AsyncList<$<ItemType>>*>(_items);
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
			return asyncItemsList().getItems(index,count).toVoid();
		} else {
			return Promise<void>::resolve();
		}
	}



	template<typename ItemType>
	bool SpecialTrackCollection<ItemType>::tracksAreEmpty() const {
		return _items.index() == 0 || _items.index() == 1;
	}

	template<typename ItemType>
	bool SpecialTrackCollection<ItemType>::tracksAreAsync() const {
		return _items.index() == 3;
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
	AsyncList<$<ItemType>>& SpecialTrackCollection<ItemType>::asyncItemsList() {
		return *std::get<AsyncList<$<ItemType>>*>(_items);
	}

	template<typename ItemType>
	const AsyncList<$<ItemType>>& SpecialTrackCollection<ItemType>::asyncItemsList() const {
		return *std::get<AsyncList<$<ItemType>>*>(_items);
	}

	template<typename ItemType>
	void SpecialTrackCollection<ItemType>::makeTracksAsync() {
		if(tracksAreAsync()) {
			return;
		} else if(tracksAreEmpty()) {
			if(_items.index() == 0) {
				_items = new AsyncList<$<ItemType>>({
					.delegate=this,
					.chunkSize=24
				});
			} else {
				auto& tracks = std::get<EmptyTracks>(_items);
				_items = new AsyncList<$<ItemType>>({
					.delegate=this,
					.chunkSize=24,
					.initialSize=tracks.total
				});
			}
		} else {
			auto& tracks = itemsList();
			_items = new AsyncList<$<ItemType>>({
				.delegate=this,
				.chunkSize=24,
				.initialItemsOffset=0,
				.initialItems=tracks,
				.initialSize=tracks.size()
			});
		}
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

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
	_SpecialTrackCollection<ItemType>::_SpecialTrackCollection(MediaProvider* provider, Data data)
	: TrackCollection(provider, data), _items(constructItems(data.tracks)) {
		//
	}

	template<typename ItemType>
	std::variant<typename _SpecialTrackCollection<ItemType>::EmptyTracks,LinkedList<ItemType>,AsyncList<ItemType>> _SpecialTrackCollection<ItemType>::constructItems(typename Data::Tracks tracks) {
		if(tracks.items.size() == 0) {
			return EmptyTracks{
				.total = tracks.total
			};
		} else if(tracks.offset == 0 && tracks.items.size() == tracks.total) {
			return tracks.items;
		} else {
			return AsyncList<ItemType>({
				.delegate=this,
				.chunkSize=24,
				.initialItemsOffset=tracks.offset,
				.initialItems=tracks.items,
				.initialSize=tracks.total
			});
		}
	}

	template<typename ItemType>
	$<ItemType> SpecialTrackCollection<ItemType>::itemAt(size_t index) {
		if(this->tracksAreEmpty()) {
			return nullptr;
		} else if(this->tracksAreAsync()) {
			return this->asyncItemsList().itemAt(index).value_or(nullptr);
		} else {
			auto& items = this->itemsList();
			if(index >= items.size()) {
				return nullptr;
			}
			return *std::next(items.begin(), index);
		}
	}

	template<typename ItemType>
	$<TrackCollectionItem> _SpecialTrackCollection<ItemType>::itemAt(size_t index) {
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
	$<const ItemType> SpecialTrackCollection<ItemType>::itemAt(size_t index) const {
		if(this->tracksAreEmpty()) {
			return nullptr;
		} else if(this->tracksAreAsync()) {
			return std::static_pointer_cast<const ItemType>(this->asyncItemsList().itemAt(index).value_or(nullptr));
		} else {
			auto& items = this->itemsList();
			if(index >= items.size()) {
				return nullptr;
			}
			return std::static_pointer_cast<const ItemType>(*std::next(items.begin(), index));
		}
	}

	template<typename ItemType>
	$<const TrackCollectionItem> _SpecialTrackCollection<ItemType>::itemAt(size_t index) const {
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
	Promise<$<ItemType>> SpecialTrackCollection<ItemType>::getItem(size_t index) {
		if(!this->tracksAreEmpty() && !this->tracksAreAsync()) {
			auto& items = this->itemsList();
			if(index >= items.size()) {
				return Promise<$<ItemType>>::resolve(nullptr);
			}
			return Promise<$<ItemType>>::resolve(*std::next(items.begin(), index));
		}
		this->makeTracksAsync();
		return this->asyncItemsList().getItem(index).template map<$<ItemType>>(nullptr, [](auto item) {
			return item.value_or(nullptr);
		});
	}

	template<typename ItemType>
	Promise<$<TrackCollectionItem>> _SpecialTrackCollection<ItemType>::getItem(size_t index) {
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
	Promise<LinkedList<$<ItemType>>> SpecialTrackCollection<ItemType>::getItems(size_t index, size_t count) {
		if(!this->tracksAreEmpty() && !this->tracksAreAsync()) {
			auto& items = this->itemsList();
			if(index >= items.size()) {
				return Promise<$<ItemType>>::resolve(LinkedList<$<ItemType>>());
			}
			auto startIt = std::next(items.begin(), index);
			auto endIt = ((index+count) < items.size()) ? std::next(startIt, count) : items.end();
			return Promise<$<ItemType>>::resolve(LinkedList<$<ItemType>>(startIt, endIt));
		}
		this->makeTracksAsync();
		return this->asyncItemsList().getItems(index, count);
	}

	template<typename ItemType>
	Promise<LinkedList<$<TrackCollectionItem>>> _SpecialTrackCollection<ItemType>::getItems(size_t index, size_t count) {
		if(!tracksAreEmpty() && !tracksAreAsync()) {
			auto& items = itemsList();
			if(index >= items.size()) {
				return Promise<$<ItemType>>::resolve(LinkedList<$<ItemType>>());
			}
			auto startIt = std::next(items.begin(), index);
			auto endIt = ((index+count) < items.size()) ? std::next(startIt, count) : items.end();
			LinkedList<$<TrackCollectionItem>> selectedItems;
			for(auto it=startIt; it!=endIt; it++) {
				selectedItems.pushBack(std::static_pointer_cast<$<TrackCollectionItem>>(*it));
			}
			return Promise<$<ItemType>>::resolve(selectedItems);
		}
		makeTracksAsync();
		return this->asyncItemsList().getItems(index, count).template map<LinkedList<$<TrackCollectionItem>>>(nullptr, [](auto items) {
			return items.template map<$<TrackCollectionItem>>([](auto& item) {
				return std::static_pointer_cast<TrackCollectionItem>(item);
			});
		});
	}

	template<typename ItemType>
	Generator<LinkedList<$<ItemType>>,void> SpecialTrackCollection<ItemType>::generateItems(size_t startIndex) {
		if(this->tracksAreEmpty()) {
			this->makeTracksAsync();
		}
		if(this->tracksAreAsync()) {
			return this->asyncItemsList().generateItems(startIndex);
		} else {
			size_t chunkSize = 24;
			auto index = new$<size_t>(startIndex);
			auto items = new$<LinkedList<$<ItemType>>>(this->itemsList());
			return Generator<LinkedList<$<ItemType>>,void>([=]() {
				using YieldResult = typename Generator<LinkedList<$<ItemType>>,void>::YieldResult;
				LinkedList<$<ItemType>> genItems;
				while(genItems.size() < chunkSize && items.size() > 0) {
					genItems.pushBack(std::move(items.front()));
					items.popFront();
				}
				if(items.size() == 0) {
					return Promise<YieldResult>::resolve({
						.value=genItems,
						.done=true
					});
				} else {
					return Promise<YieldResult>::resolve({
						.value=genItems,
						.done=false
					});
				}
			});
		}
	}

	template<typename ItemType>
	Generator<LinkedList<$<TrackCollectionItem>>,void> _SpecialTrackCollection<ItemType>::generateItems(size_t startIndex) {
		if(tracksAreEmpty()) {
			makeTracksAsync();
		}
		if(tracksAreAsync()) {
			return asyncItemsList().generateItems(startIndex).template map<LinkedList<$<TrackCollectionItem>>>([](auto items) {
				return items.template map<$<TrackCollectionItem>>([](auto& item) {
					return std::static_pointer_cast<$<TrackCollectionItem>>(item);
				});
			});
		} else {
			size_t chunkSize = 24;
			auto index = new$<size_t>(startIndex);
			auto items = new$<LinkedList<$<ItemType>>>(itemsList());
			return Generator<LinkedList<$<TrackCollectionItem>>,void>([=]() {
				using YieldResult = typename Generator<LinkedList<$<ItemType>>,void>::YieldResult;
				LinkedList<$<ItemType>> genItems;
				while(genItems.size() < chunkSize && items.size() > 0) {
					genItems.pushBack(std::static_pointer_cast<$<TrackCollectionItem>>(items.front()));
					items.popFront();
				}
				if(items.size() == 0) {
					return Promise<YieldResult>::resolve({
						.value=genItems,
						.done=true
					});
				} else {
					return Promise<YieldResult>::resolve({
						.value=genItems,
						.done=false
					});
				}
			});
		}
	}

	template<typename ItemType>
	size_t _SpecialTrackCollection<ItemType>::itemCount() const {
		if(tracksAreEmpty()) {
			return std::get<EmptyTracks>(_items).total;
		} else if(tracksAreAsync()) {
			return asyncItemsList().size();
		} else {
			return itemsList().size();
		}
	}
	
	template<typename ItemType>
	Promise<void> _SpecialTrackCollection<ItemType>::loadItems(size_t index, size_t count) {
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
	bool _SpecialTrackCollection<ItemType>::tracksAreEmpty() const {
		return _items.index() == 0;
	}

	template<typename ItemType>
	bool _SpecialTrackCollection<ItemType>::tracksAreAsync() const {
		return _items.index() == 2;
	}

	template<typename ItemType>
	LinkedList<ItemType>& _SpecialTrackCollection<ItemType>::itemsList() {
		return std::get<LinkedList<ItemType>>(_items);
	}

	template<typename ItemType>
	const LinkedList<ItemType>& _SpecialTrackCollection<ItemType>::itemsList() const {
		return std::get<LinkedList<ItemType>>(_items);
	}

	template<typename ItemType>
	AsyncList<ItemType>& _SpecialTrackCollection<ItemType>::asyncItemsList() {
		return std::get<AsyncList<ItemType>>(_items);
	}

	template<typename ItemType>
	const AsyncList<ItemType>& _SpecialTrackCollection<ItemType>::asyncItemsList() const {
		return std::get<AsyncList<ItemType>>(_items);
	}

	template<typename ItemType>
	void _SpecialTrackCollection<ItemType>::makeTracksAsync() {
		if(tracksAreAsync()) {
			return;
		} else if(tracksAreEmpty()) {
			auto& tracks = std::get<EmptyTracks>(_items);
			_items = AsyncList<ItemType>({
				.delegate=this,
				.chunkSize=24,
				.initialSize=tracks.total
			});
		} else {
			auto& tracks = itemsList();
			_items = AsyncList<ItemType>({
				.delegate=this,
				.chunkSize=24,
				.initialItemsOffset=0,
				.initialItems=tracks,
				.initialSize=tracks.size()
			});
		}
	}
}

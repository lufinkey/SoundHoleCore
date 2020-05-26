//
//  TrackCollection.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <variant>
#include <soundhole/common.hpp>
#include "MediaItem.hpp"
#include "Track.hpp"

namespace sh {
	class TrackCollection;


	class TrackCollectionItem {
	public:
		struct Data {
			$<Track> track;
		};
		
		virtual ~TrackCollectionItem() {};
		
		$<Track> track();
		$<const Track> track() const;
		w$<TrackCollection> context();
		w$<const TrackCollection> context() const;
		
		Optional<size_t> indexInContext() const;
		
		virtual bool matchesItem(const TrackCollectionItem* item) const = 0;
		
		Data toData() const;
		
		virtual Json toJson() const;
		
	protected:
		TrackCollectionItem($<TrackCollectionItem>& ptr, $<TrackCollection> context, Data data);
		TrackCollectionItem($<TrackCollectionItem>& ptr, $<TrackCollection> context, Json json, MediaProviderStash* stash);
		
		$<TrackCollectionItem> self();
		$<const TrackCollectionItem> self() const;
		template<typename T>
		$<T> selfAs();
		template<typename T>
		$<const T> selfAs() const;
		
		w$<TrackCollectionItem> weakSelf;
		w$<TrackCollection> _context;
		$<Track> _track;
	};



	class TrackCollection: public MediaItem {
	public:
		using MediaItem::MediaItem;
		using ItemGenerator = ContinuousGenerator<LinkedList<$<TrackCollectionItem>>,void>;
		
		virtual Optional<size_t> indexOfItem(const TrackCollectionItem* item) const = 0;
		virtual Optional<size_t> indexOfItemInstance(const TrackCollectionItem* item) const = 0;
		virtual $<TrackCollectionItem> itemAt(size_t index) = 0;
		virtual $<const TrackCollectionItem> itemAt(size_t index) const = 0;
		virtual Promise<$<TrackCollectionItem>> getItem(size_t index) = 0;
		virtual Promise<LinkedList<$<TrackCollectionItem>>> getItems(size_t index, size_t count) = 0;
		virtual ItemGenerator generateItems(size_t startIndex=0) = 0;
		virtual $<TrackCollectionItem> itemFromJson(Json json, MediaProviderStash* stash) = 0;
		
		virtual Optional<size_t> itemCount() const = 0;
		virtual Promise<void> loadItems(size_t index, size_t count) = 0;
		
		virtual void forEach(Function<void($<TrackCollectionItem>,size_t)>) = 0;
		virtual void forEach(Function<void($<const TrackCollectionItem>,size_t)>) const = 0;
		virtual void forEachInRange(size_t startIndex, size_t endIndex, Function<void($<TrackCollectionItem>,size_t)>) = 0;
		virtual void forEachInRange(size_t startIndex, size_t endIndex, Function<void($<const TrackCollectionItem>,size_t)>) const = 0;
		
		virtual Json toJson() const override final;
		struct ToJsonOptions {
			size_t tracksOffset = 0;
			size_t tracksLimit = (size_t)-1;
		};
		virtual Json toJson(ToJsonOptions options) const;
	};



	template<typename ItemType>
	class SpecialTrackCollection: public TrackCollection,
	protected AsyncList<$<ItemType>>::Delegate {
	public:
		using Item = ItemType;
		
		class MutatorDelegate {
		public:
			using Mutator = typename AsyncList<$<ItemType>>::Mutator;
			virtual ~MutatorDelegate() {}
			
			virtual Promise<void> loadItems(Mutator* mutator, size_t index, size_t count) = 0;
		};
		
		struct Data: public TrackCollection::Data {
			struct Tracks {
				size_t total;
				size_t offset = 0;
				LinkedList<typename ItemType::Data> items;
			};
			
			Optional<Tracks> tracks;
		};
		
		virtual ~SpecialTrackCollection();
		
		virtual Optional<size_t> indexOfItem(const TrackCollectionItem* item) const override;
		Optional<size_t> indexOfItem(const ItemType* item) const;
		virtual Optional<size_t> indexOfItemInstance(const TrackCollectionItem* item) const override;
		Optional<size_t> indexOfItemInstance(const ItemType* item) const;
		
		virtual $<TrackCollectionItem> itemAt(size_t index) override final;
		virtual $<const TrackCollectionItem> itemAt(size_t index) const override final;
		virtual Promise<$<TrackCollectionItem>> getItem(size_t index) override final;
		virtual Promise<LinkedList<$<TrackCollectionItem>>> getItems(size_t index, size_t count) override final;
		virtual ItemGenerator generateItems(size_t startIndex=0) override final;
		virtual $<TrackCollectionItem> itemFromJson(Json json, MediaProviderStash* stash) override final;
		
		virtual Optional<size_t> itemCount() const override final;
		virtual Promise<void> loadItems(size_t index, size_t count) override final;
		
		virtual void forEach(Function<void($<TrackCollectionItem>,size_t)>) override final;
		virtual void forEach(Function<void($<const TrackCollectionItem>,size_t)>) const override final;
		virtual void forEachInRange(size_t startIndex, size_t endIndex, Function<void($<TrackCollectionItem>,size_t)>) override final;
		virtual void forEachInRange(size_t startIndex, size_t endIndex, Function<void($<const TrackCollectionItem>,size_t)>) const override final;
		
		void applyData(const Data& data);
		
		struct DataOptions {
			size_t tracksOffset = 0;
			size_t tracksLimit = (size_t)-1;
		};
		Data toData(DataOptions options = DataOptions()) const;
		virtual Json toJson(ToJsonOptions options) const override;
		
	protected:
		SpecialTrackCollection($<MediaItem>& ptr, MediaProvider* provider, Data data);
		SpecialTrackCollection($<MediaItem>& ptr, Json json, MediaProviderStash* stash);
		SpecialTrackCollection($<MediaItem>& ptr, MediaProvider* provider, Data data, MutatorDelegate* mutatorDelegate, bool autoDeleteMutatorDelegate);
		SpecialTrackCollection($<MediaItem>& ptr, Json json, MediaProviderStash* stash, MutatorDelegate* mutatorDelegate, bool autoDeleteMutatorDelegate);
		
		virtual bool areAsyncListItemsEqual(const AsyncList<$<ItemType>>* list, const $<ItemType>& item1, const $<ItemType>& item2) const override;
		virtual Promise<void> loadAsyncListItems(typename AsyncList<$<ItemType>>::Mutator* mutator, size_t index, size_t count) override;
		
		virtual MutatorDelegate* createMutatorDelegate();
		MutatorDelegate* mutatorDelegate();
		
		inline bool tracksAreEmpty() const;
		inline bool tracksAreAsync() const;
		inline LinkedList<$<ItemType>>& itemsList();
		inline const LinkedList<$<ItemType>>& itemsList() const;
		inline $<AsyncList<$<ItemType>>> asyncItemsList();
		inline $<const AsyncList<$<ItemType>>> asyncItemsList() const;
		void makeTracksAsync();
		
		struct EmptyTracks {
			size_t total;
		};
		
		std::variant<std::nullptr_t,EmptyTracks,LinkedList<$<ItemType>>,$<AsyncList<$<ItemType>>>> _items;
		std::variant<std::nullptr_t,EmptyTracks,LinkedList<$<ItemType>>,$<AsyncList<$<ItemType>>>> constructItems(Optional<typename Data::Tracks> tracks);
		
		MutatorDelegate* _mutatorDelegate;
		bool autoDeleteMutatorDelegate;
	};


	template<typename Context>
	class SpecialTrackCollectionItem: public TrackCollectionItem {
	public:
		using TrackCollectionItem::TrackCollectionItem;
		
		w$<Context> context();
		w$<const Context> context() const;
	};
}

#include "TrackCollection.impl.hpp"

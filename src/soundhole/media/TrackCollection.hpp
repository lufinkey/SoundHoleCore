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
		
		TrackCollectionItem($<TrackCollection> context, Data data);
		virtual ~TrackCollectionItem() {};
		
		$<Track> track();
		$<const Track> track() const;
		w$<TrackCollection> context();
		w$<const TrackCollection> context() const;
		
		Optional<size_t> indexInContext() const;
		
		virtual bool matchesItem(const TrackCollectionItem* item) const = 0;
		
	protected:
		w$<TrackCollection> _context;
		$<Track> _track;
	};



	class TrackCollection: public MediaItem {
	public:
		using MediaItem::MediaItem;
		
		virtual Optional<size_t> indexOfItem(const TrackCollectionItem* item) const = 0;
		virtual $<TrackCollectionItem> itemAt(size_t index) = 0;
		virtual $<const TrackCollectionItem> itemAt(size_t index) const = 0;
		virtual Promise<$<TrackCollectionItem>> getItem(size_t index) = 0;
		virtual Promise<LinkedList<$<TrackCollectionItem>>> getItems(size_t index, size_t count) = 0;
		virtual Generator<LinkedList<$<TrackCollectionItem>>,void> generateItems(size_t startIndex=0) = 0;
		
		virtual Optional<size_t> itemCount() const = 0;
		
		virtual Promise<void> loadItems(size_t index, size_t count) = 0;
	};



	template<typename ItemType>
	class SpecialTrackCollection: public TrackCollection,
	public std::enable_shared_from_this<SpecialTrackCollection<ItemType>>,
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
		
		SpecialTrackCollection(MediaProvider* provider, Data data);
		virtual ~SpecialTrackCollection();
		
		virtual Optional<size_t> indexOfItem(const TrackCollectionItem* item) const override;
		Optional<size_t> indexOfItem(const ItemType* item) const;
		
		virtual $<TrackCollectionItem> itemAt(size_t index) override final;
		virtual $<const TrackCollectionItem> itemAt(size_t index) const override final;
		virtual Promise<$<TrackCollectionItem>> getItem(size_t index) override final;
		virtual Promise<LinkedList<$<TrackCollectionItem>>> getItems(size_t index, size_t count) override final;
		virtual Generator<LinkedList<$<TrackCollectionItem>>,void> generateItems(size_t startIndex=0) override final;
		
		virtual Optional<size_t> itemCount() const override;
		
		virtual Promise<void> loadItems(size_t index, size_t count) override;
		
	protected:
		inline $<SpecialTrackCollection<ItemType>> self();
		inline $<const SpecialTrackCollection<ItemType>> self() const;
		
		virtual bool areAsyncListItemsEqual(const AsyncList<$<ItemType>>* list, const $<ItemType>& item1, const $<ItemType>& item2) const override;
		virtual Promise<void> loadAsyncListItems(typename AsyncList<$<ItemType>>::Mutator* mutator, size_t index, size_t count) override;
		
		virtual MutatorDelegate* createMutatorDelegate() = 0;
		MutatorDelegate* mutatorDelegate();
		
		inline bool tracksAreEmpty() const;
		inline bool tracksAreAsync() const;
		inline LinkedList<$<ItemType>>& itemsList();
		inline const LinkedList<$<ItemType>>& itemsList() const;
		inline AsyncList<$<ItemType>>& asyncItemsList();
		inline const AsyncList<$<ItemType>>& asyncItemsList() const;
		void makeTracksAsync();
		
		struct EmptyTracks {
			size_t total;
		};
		
		std::variant<std::nullptr_t,EmptyTracks,LinkedList<$<ItemType>>,AsyncList<$<ItemType>>*> _items;
		std::variant<std::nullptr_t,EmptyTracks,LinkedList<$<ItemType>>,AsyncList<$<ItemType>>*> constructItems(Optional<typename Data::Tracks> tracks);
		
		MutatorDelegate* _mutatorDelegate;
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

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
		
		TrackCollectionItem(w$<TrackCollection> context, Data data);
		virtual ~TrackCollectionItem() {};
		
		$<Track> track();
		$<const Track> track() const;
		w$<TrackCollection> context();
		w$<const TrackCollection> context() const;
		
		Optional<size_t> indexInContext() const;
		
		virtual bool matchesItem(const TrackCollectionItem* item) const = 0;
		
	protected:
		$<Track> _track;
		w$<TrackCollection> _context;
	};



	class TrackCollection: public MediaItem {
	public:
		using MediaItem::MediaItem;
		
		virtual $<TrackCollectionItem> itemAt(size_t index) = 0;
		virtual $<const TrackCollectionItem> itemAt(size_t index) const = 0;
		virtual Promise<$<TrackCollectionItem>> getItem(size_t index) = 0;
		virtual Promise<LinkedList<$<TrackCollectionItem>>> getItems(size_t index, size_t count) = 0;
		virtual Generator<LinkedList<$<TrackCollectionItem>>,void> generateItems(size_t startIndex=0) = 0;
		
		virtual size_t itemCount() const = 0;
		
		virtual Promise<void> loadItems(size_t index, size_t count) = 0;
	};



	template<typename ItemType>
	class _SpecialTrackCollection: public TrackCollection,
	public std::enable_shared_from_this<_SpecialTrackCollection<ItemType>>,
	protected AsyncList<ItemType>::Delegate {
	public:
		using Item = ItemType;
		
		struct Data: public TrackCollection::Data {
			struct Tracks {
				size_t total;
				size_t offset = 0;
				LinkedList<typename ItemType::Data> items;
			};
			
			Tracks tracks;
		};
		
		_SpecialTrackCollection(MediaProvider* provider, Data data);
		
		virtual size_t itemCount() const override;
		
		virtual Promise<void> loadItems(size_t index, size_t count) override;
		
	protected:
		virtual $<TrackCollectionItem> itemAt(size_t index) override;
		virtual $<const TrackCollectionItem> itemAt(size_t index) const override;
		virtual Promise<$<TrackCollectionItem>> getItem(size_t index) override;
		virtual Promise<LinkedList<$<TrackCollectionItem>>> getItems(size_t index, size_t count) override;
		virtual Generator<LinkedList<$<TrackCollectionItem>>,void> generateItems(size_t startIndex=0) override;
		
		inline bool tracksAreEmpty() const;
		inline bool tracksAreAsync() const;
		inline LinkedList<ItemType>& itemsList();
		inline const LinkedList<ItemType>& itemsList() const;
		inline AsyncList<ItemType>& asyncItemsList();
		inline const AsyncList<ItemType>& asyncItemsList() const;
		void makeTracksAsync();
		
		struct EmptyTracks {
			size_t total;
		};
		
		std::variant<EmptyTracks,LinkedList<ItemType>,AsyncList<ItemType>> _items;
		
	private:
		std::variant<EmptyTracks,LinkedList<ItemType>,AsyncList<ItemType>> constructItems(typename Data::Tracks tracks);
	};



	template<typename ItemType>
	class SpecialTrackCollection: public _SpecialTrackCollection<ItemType> {
	public:
		using _SpecialTrackCollection<ItemType>::_SpecialTrackCollection;
		
		$<ItemType> itemAt(size_t index);
		$<const ItemType> itemAt(size_t index) const;
		Promise<$<ItemType>> getItem(size_t index);
		Promise<LinkedList<$<ItemType>>> getItems(size_t index, size_t count);
		Generator<LinkedList<$<ItemType>>,void> generateItems(size_t startIndex=0);
	};
}

#include "TrackCollection.impl.hpp"

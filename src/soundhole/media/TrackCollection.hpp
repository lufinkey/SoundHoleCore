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

#ifdef __OBJC__
#import <Foundation/Foundation.h>
@protocol SHTrackCollectionSubscriber;
#endif

namespace sh {
	class TrackCollection;
	class MediaDatabase;


	class TrackCollectionItem: public std::enable_shared_from_this<TrackCollectionItem> {
	public:
		struct Data {
			$<Track> track;
		};
		
		TrackCollectionItem($<TrackCollection> context, const Data& data);
		TrackCollectionItem($<TrackCollection> context, const Json& json, MediaProviderStash* stash);
		virtual ~TrackCollectionItem() {};
		
		$<Track> track();
		$<const Track> track() const;
		w$<TrackCollection> context();
		w$<const TrackCollection> context() const;
		
		Optional<size_t> indexInContext() const;
		
		virtual bool matchesItem(const TrackCollectionItem* item) const = 0;
		virtual void merge(const TrackCollectionItem* item);
		
		Data toData() const;
		
		virtual Json toJson() const;
		
	protected:
		w$<TrackCollection> _context;
		$<Track> _track;
	};



	class TrackCollection: public MediaItem {
	public:
		using MediaItem::MediaItem;
		using ItemGenerator = ContinuousGenerator<LinkedList<$<TrackCollectionItem>>,void>;
		using ItemIndexMarker = AsyncListIndexMarker;
		using ItemIndexMarkerState = AsyncListIndexMarkerState;
		using ItemsMutation = AsyncListMutation;
		using ItemsListChange = AsyncListChange;
		
		class Subscriber {
			friend class TrackCollection;
		public:
			virtual ~Subscriber() = default;
			virtual void onTrackCollectionMutations($<TrackCollection> collection, const ItemsListChange& change) = 0;
		};
		class AutoDeletedSubscriber: public Subscriber {
			//
		};
		
		struct LoadItemOptions {
			MediaDatabase* database = nullptr;
			bool forceReload = false;
			bool trackIndexChanges = false;
			
			std::map<String,Any> toDict() const;
			static LoadItemOptions fromDict(const std::map<String,Any>& dict);
			
			static LoadItemOptions defaultOptions();
		};
		
		virtual String versionId() const;
		
		virtual Optional<size_t> indexOfItem(const TrackCollectionItem* item) const = 0;
		virtual Optional<size_t> indexOfItemInstance(const TrackCollectionItem* item) const = 0;
		virtual $<TrackCollectionItem> itemAt(size_t index) = 0;
		virtual $<const TrackCollectionItem> itemAt(size_t index) const = 0;
		virtual Promise<$<TrackCollectionItem>> getItem(size_t index, LoadItemOptions options = LoadItemOptions::defaultOptions()) = 0;
		virtual Promise<LinkedList<$<TrackCollectionItem>>> getItems(size_t index, size_t count, LoadItemOptions options = LoadItemOptions::defaultOptions()) = 0;
		virtual ItemGenerator generateItems(size_t startIndex=0, LoadItemOptions options = LoadItemOptions::defaultOptions()) = 0;
		virtual $<TrackCollectionItem> itemFromJson(const Json& json, MediaProviderStash* stash) = 0;
		
		virtual Optional<size_t> itemCount() const = 0;
		virtual size_t itemCapacity() const = 0;
		virtual Promise<void> loadItems(size_t index, size_t count, LoadItemOptions options = LoadItemOptions::defaultOptions()) = 0;
		
		virtual void forEach(Function<void($<TrackCollectionItem>,size_t)>) = 0;
		virtual void forEach(Function<void($<const TrackCollectionItem>,size_t)>) const = 0;
		virtual void forEachInRange(size_t startIndex, size_t endIndex, Function<void($<TrackCollectionItem>,size_t)>) = 0;
		virtual void forEachInRange(size_t startIndex, size_t endIndex, Function<void($<const TrackCollectionItem>,size_t)>) const = 0;
		
		virtual void invalidateItems(size_t startIndex, size_t endIndex) = 0;
		virtual void invalidateAllItems() = 0;
		virtual void resetItems() = 0;
		
		virtual ItemIndexMarker watchIndex(size_t index) = 0;
		virtual void unwatchIndex(ItemIndexMarker indexMarker) = 0;
		
		virtual void subscribe(Subscriber* subscriber) = 0;
		virtual void unsubscribe(Subscriber* subscriber) = 0;
		#ifdef __OBJC__
		void subscribe(id<SHTrackCollectionSubscriber> subscriber);
		void unsubscribe(id<SHTrackCollectionSubscriber> subscriber);
		#endif
		
		virtual void lockItems(Function<void()> work) = 0;
		
		virtual Json toJson() const override final;
		struct ToJsonOptions {
			size_t tracksOffset = 0;
			size_t tracksLimit = (size_t)-1;
		};
		virtual Json toJson(const ToJsonOptions& options) const;
		
	protected:
		virtual LinkedList<Subscriber*> subscribers() const;
	};



	template<typename ItemType>
	class SpecialTrackCollection: public TrackCollection,
	protected AsyncList<$<ItemType>>::Delegate {
	public:
		using Item = ItemType;
		
		class MutatorDelegate {
		public:
			using Mutator = typename AsyncList<$<ItemType>>::Mutator;
			using LoadItemOptions = TrackCollection::LoadItemOptions;
			
			virtual ~MutatorDelegate() {}
			
			virtual size_t getChunkSize() const = 0;
			
			virtual Promise<void> loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) = 0;
		};
		
		struct Data: public TrackCollection::Data {
			struct Tracks {
				size_t total;
				size_t offset = 0;
				LinkedList<typename ItemType::Data> items;
			};
			
			String versionId;
			Optional<Tracks> tracks;
		};
		
		SpecialTrackCollection(MediaProvider* provider, const Data& data);
		SpecialTrackCollection(const Json& json, MediaProviderStash* stash);
		SpecialTrackCollection(MediaProvider* provider, const Data& data, MutatorDelegate* mutatorDelegate, bool autoDeleteMutatorDelegate);
		SpecialTrackCollection(const Json& json, MediaProviderStash* stash, MutatorDelegate* mutatorDelegate, bool autoDeleteMutatorDelegate);
		virtual ~SpecialTrackCollection();
		
		virtual String versionId() const override;
		
		virtual Optional<size_t> indexOfItem(const TrackCollectionItem* item) const override;
		Optional<size_t> indexOfItem(const ItemType* item) const;
		virtual Optional<size_t> indexOfItemInstance(const TrackCollectionItem* item) const override;
		Optional<size_t> indexOfItemInstance(const ItemType* item) const;
		
		virtual $<TrackCollectionItem> itemAt(size_t index) override final;
		virtual $<const TrackCollectionItem> itemAt(size_t index) const override final;
		virtual Promise<$<TrackCollectionItem>> getItem(size_t index, LoadItemOptions options = LoadItemOptions::defaultOptions()) override final;
		virtual Promise<LinkedList<$<TrackCollectionItem>>> getItems(size_t index, size_t count, LoadItemOptions options = LoadItemOptions::defaultOptions()) override final;
		virtual ItemGenerator generateItems(size_t startIndex=0, LoadItemOptions options = LoadItemOptions::defaultOptions()) override final;
		virtual $<TrackCollectionItem> itemFromJson(const Json& json, MediaProviderStash* stash) override final;
		
		virtual Optional<size_t> itemCount() const override final;
		virtual size_t itemCapacity() const override final;
		virtual Promise<void> loadItems(size_t index, size_t count, LoadItemOptions options = LoadItemOptions::defaultOptions()) override final;
		
		virtual void forEach(Function<void($<TrackCollectionItem>,size_t)>) override final;
		virtual void forEach(Function<void($<const TrackCollectionItem>,size_t)>) const override final;
		virtual void forEachInRange(size_t startIndex, size_t endIndex, Function<void($<TrackCollectionItem>,size_t)>) override final;
		virtual void forEachInRange(size_t startIndex, size_t endIndex, Function<void($<const TrackCollectionItem>,size_t)>) const override final;
		
		virtual void invalidateItems(size_t startIndex, size_t endIndex) override;
		virtual void invalidateAllItems() override;
		virtual void resetItems() override;
		
		virtual ItemIndexMarker watchIndex(size_t index) override;
		virtual void unwatchIndex(ItemIndexMarker indexMarker) override;
		
		virtual void subscribe(Subscriber* subscriber) override;
		virtual void unsubscribe(Subscriber* subscriber) override;
		
		virtual void lockItems(Function<void()> lock) override;
		
		void applyData(const Data& data);
		
		struct DataOptions {
			size_t tracksOffset = 0;
			size_t tracksLimit = (size_t)-1;
		};
		Data toData(const DataOptions& options = DataOptions()) const;
		virtual Json toJson(const ToJsonOptions& options) const override;
		
	protected:
		virtual size_t getAsyncListChunkSize(const AsyncList<$<ItemType>>* list) const override;
		virtual bool areAsyncListItemsEqual(const AsyncList<$<ItemType>>* list, const $<ItemType>& item1, const $<ItemType>& item2) const override;
		virtual void mergeAsyncListItem(const AsyncList<$<ItemType>>* list, $<ItemType>& overwritingItem, $<ItemType>& existingItem) override;
		virtual Promise<void> loadAsyncListItems(typename AsyncList<$<ItemType>>::Mutator* mutator, size_t index, size_t count, std::map<String,Any> options) override;
		virtual void onAsyncListMutations(const AsyncList<$<ItemType>>* list, AsyncListChange change) override;
		
		virtual MutatorDelegate* createMutatorDelegate();
		MutatorDelegate* mutatorDelegate();
		virtual LinkedList<Subscriber*> subscribers() const override;
		
		inline bool tracksAreEmpty() const;
		inline bool tracksAreAsync() const;
		inline LinkedList<$<ItemType>>& itemsList();
		inline const LinkedList<$<ItemType>>& itemsList() const;
		inline $<AsyncList<$<ItemType>>> asyncItemsList();
		inline $<const AsyncList<$<ItemType>>> asyncItemsList() const;
		void makeTracksAsync();
		
		String _versionId;
		
		struct EmptyTracks {
			size_t total;
		};
		
		std::variant<std::nullptr_t,EmptyTracks,LinkedList<$<ItemType>>,$<AsyncList<$<ItemType>>>> _items;
		std::variant<std::nullptr_t,EmptyTracks,LinkedList<$<ItemType>>,$<AsyncList<$<ItemType>>>> constructItems(const Optional<typename Data::Tracks>& tracks);
		
		void lazyLoadContentIfNeeded() const;
		
		MutatorDelegate* _mutatorDelegate;
		mutable Function<void()> _lazyContentLoader;
		
		LinkedList<Subscriber*> _subscribers;
		
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



#ifdef __OBJC__

@protocol SHTrackCollectionSubscriber <NSObject>
@optional
-(void)trackCollection:(fgl::$<sh::TrackCollection>)collection didMutate:(sh::TrackCollection::ItemsListChange)change;
@end

#endif

#include "TrackCollection.impl.hpp"

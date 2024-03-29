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
		using IndexMarker = AsyncListIndexMarker;
		
		struct Data {
			$<Track> track;

			static Data fromJson(const Json& json, MediaProviderStash* stash);
		};
		
		TrackCollectionItem($<TrackCollection> context, const Data& data);
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
			virtual ~Subscriber();
			virtual void onTrackCollectionMutations($<TrackCollection> collection, const ItemsListChange& change) = 0;
		};
		class AutoDeletedSubscriber: public Subscriber {
			//
		};
		
		struct LoadItemOptions {
			MediaDatabase* database = nullptr;
			bool offline = false;
			bool forceReload = false;
			bool trackIndexChanges = false;
			
			Map<String,Any> toMap() const;
			static LoadItemOptions fromMap(const Map<String,Any>&);
			
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
		virtual $<TrackCollectionItem> createCollectionItem(const Json& json, MediaProviderStash* stash) = 0;
		
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
		virtual ItemIndexMarker watchRemovedIndex(size_t index) = 0;
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
			size_t itemsStartIndex = 0;
			size_t itemsEndIndex = (size_t)-1;
			size_t itemsLimit = (size_t)-1;
		};
		virtual Json toJson(const ToJsonOptions& options) const;
		
	protected:
		virtual LinkedList<Subscriber*> subscribers() const = 0;
	};



	template<typename ItemType>
	class SpecialTrackCollection: public TrackCollection,
	private AsyncList<$<TrackCollectionItem>,$<Track>>::Delegate {
	public:
		using Item = ItemType;
		using AsyncList = AsyncList<$<TrackCollectionItem>,$<Track>>;
		
		class Mutator {
			friend class SpecialTrackCollection<ItemType>;
		public:
			Mutator(AsyncList::Mutator*);
			
			AsyncList* getList();
			const AsyncList* getList() const;
			
			template<typename Work>
			void lock(Work work);
			void apply(size_t index, LinkedList<$<ItemType>> items);
			void apply(std::map<size_t,$<ItemType>> items);
			void applyAndResize(size_t index, size_t listSize, LinkedList<$<ItemType>> items);
			void applyAndResize(size_t listSize, std::map<size_t,$<ItemType>> items);
			void set(size_t index, LinkedList<$<ItemType>> items);
			void insert(size_t index, LinkedList<$<ItemType>> items);
			void remove(size_t index, size_t count);
			void move(size_t index, size_t count, size_t newIndex);
			void resize(size_t count);
			void invalidate(size_t index, size_t count);
			void invalidateAll();
			void resetItems();
			void resetSize();
			void reset();
			
		private:
			AsyncList::Mutator* mutator;
		};
		
		class MutatorDelegate {
		public:
			using Mutator = Mutator;
			using LoadItemOptions = TrackCollection::LoadItemOptions;
			
			virtual ~MutatorDelegate() {}
			
			virtual size_t getChunkSize() const = 0;
			virtual Promise<void> loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) = 0;
		};
		
		struct Data: public TrackCollection::Data {
			String versionId;
			Optional<size_t> itemCount;
			Map<size_t,typename ItemType::Data> items;
			
			static Data fromJson(const Json& json, MediaProviderStash* stash);
		};
		
		SpecialTrackCollection(MediaProvider* provider, const Data& data);
		SpecialTrackCollection(MediaProvider* provider, const Data& data, MutatorDelegate* mutatorDelegate, bool autoDeleteMutatorDelegate);
		virtual ~SpecialTrackCollection();
		
		virtual String versionId() const override;
		
		virtual $<ItemType> createCollectionItem(const typename ItemType::Data& data);
		virtual $<TrackCollectionItem> createCollectionItem(const Json& json, MediaProviderStash* stash) override;
		
		virtual Optional<size_t> indexOfItem(const TrackCollectionItem* item) const override;
		Optional<size_t> indexOfItem(const ItemType* item) const;
		virtual Optional<size_t> indexOfItemInstance(const TrackCollectionItem* item) const override;
		Optional<size_t> indexOfItemInstance(const ItemType* item) const;
		
		virtual $<TrackCollectionItem> itemAt(size_t index) override final;
		virtual $<const TrackCollectionItem> itemAt(size_t index) const override final;
		virtual Promise<$<TrackCollectionItem>> getItem(size_t index, LoadItemOptions options = LoadItemOptions::defaultOptions()) override final;
		virtual Promise<LinkedList<$<TrackCollectionItem>>> getItems(size_t index, size_t count, LoadItemOptions options = LoadItemOptions::defaultOptions()) override final;
		virtual ItemGenerator generateItems(size_t startIndex=0, LoadItemOptions options = LoadItemOptions::defaultOptions()) override final;
		
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
		virtual ItemIndexMarker watchRemovedIndex(size_t index) override;
		virtual void unwatchIndex(ItemIndexMarker indexMarker) override;
		
		virtual void subscribe(Subscriber* subscriber) override;
		virtual void unsubscribe(Subscriber* subscriber) override;
		
		virtual void lockItems(Function<void()> lock) override;
		
		void applyData(const Data& data);
		
		struct DataOptions {
			size_t itemsStartIndex = 0;
			size_t itemsEndIndex = (size_t)-1;
			size_t itemsLimit = (size_t)-1;
		};
		Data toData(const DataOptions& options = DataOptions()) const;
		virtual Json toJson(const ToJsonOptions& options) const override;
		
	protected:
		virtual size_t getAsyncListChunkSize(const AsyncList* list) const override;
		virtual bool areAsyncListItemsEqual(const AsyncList* list, const $<TrackCollectionItem>& item1, const $<TrackCollectionItem>& item2) const override;
		virtual void mergeAsyncListItem(const AsyncList* list, $<TrackCollectionItem>& overwritingItem, $<TrackCollectionItem>& existingItem) override;
		virtual Promise<void> loadAsyncListItems(typename AsyncList::Mutator* mutator, size_t index, size_t count, Map<String,Any> options) override;
		virtual Promise<void> insertAsyncListItems(typename AsyncList::Mutator* mutator, size_t index, LinkedList<$<Track>> tracks, Map<String,Any> options) override;
		virtual Promise<void> appendAsyncListItems(typename AsyncList::Mutator* mutator, LinkedList<$<Track>> tracks, Map<String,Any> options) override;
		virtual Promise<void> removeAsyncListItems(typename AsyncList::Mutator* mutator, size_t index, size_t count, Map<String,Any> options) override;
		virtual Promise<void> moveAsyncListItems(typename AsyncList::Mutator* mutator, size_t index, size_t count, size_t newIndex, Map<String,Any> options) override;
		virtual void onAsyncListMutations(const AsyncList* list, AsyncListChange change) override;
		
		virtual MutatorDelegate* createMutatorDelegate();
		MutatorDelegate* mutatorDelegate();
		virtual LinkedList<Subscriber*> subscribers() const override;
		
		inline bool tracksAreEmpty() const;
		inline bool tracksAreAsync() const;
		inline LinkedList<$<ItemType>>& itemsList();
		inline const LinkedList<$<ItemType>>& itemsList() const;
		inline $<AsyncList> asyncItemsList();
		inline $<const AsyncList> asyncItemsList() const;
		void makeTracksAsync();
		
		String _versionId;
		
		struct EmptyTracks {
			size_t total;
		};
		using ItemsListVariant = std::variant<std::nullptr_t,EmptyTracks,LinkedList<$<ItemType>>,$<AsyncList>>;
		
		ItemsListVariant _items;
		ItemsListVariant constructItems(std::map<size_t,typename ItemType::Data> items, Optional<size_t> itemCount);
		
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

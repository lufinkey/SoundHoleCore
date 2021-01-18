//
//  MediaLibraryTracksCollection.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 6/17/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/TrackCollection.hpp>
#include <soundhole/database/MediaDatabase.hpp>
#include <soundhole/database/SQLOrder.hpp>
#include <soundhole/database/SQLOrderBy.hpp>

namespace sh {
	class MediaLibraryTracksCollection;
	class MediaLibraryProxyProvider;


	class MediaLibraryTracksCollectionItem: public SpecialTrackCollectionItem<MediaLibraryTracksCollection> {
	public:
		struct Data: public SpecialTrackCollectionItem<MediaLibraryTracksCollection>::Data {
			String addedAt;
			
			static Data fromJson(const Json& json, MediaProviderStash* stash);
		};
		
		static $<MediaLibraryTracksCollectionItem> new$($<SpecialTrackCollection<MediaLibraryTracksCollectionItem>> collection, const Data& data);
		
		MediaLibraryTracksCollectionItem($<SpecialTrackCollection<MediaLibraryTracksCollectionItem>> collection, const Data& data);
		
		const String& addedAt() const;
		
		virtual bool matchesItem(const TrackCollectionItem* item) const override;
		
		Data toData() const;
		virtual Json toJson() const override;
		
	protected:
		String _addedAt;
	};


	class MediaLibraryTracksCollection: public SpecialTrackCollection<MediaLibraryTracksCollectionItem>, public SpecialTrackCollection<MediaLibraryTracksCollectionItem>::MutatorDelegate {
		friend class MediaLibraryProxyProvider;
	public:
		using LoadItemOptions = TrackCollection::LoadItemOptions;
		static auto constexpr TYPE = "library tracks";
		
		struct Filters {
			MediaProvider* libraryProvider = nullptr;
			sql::LibraryItemOrderBy orderBy = sql::LibraryItemOrderBy::ADDED_AT;
			sql::Order order = sql::Order::DESC;
			
			static Filters fromJson(const Json& json, MediaProviderStash* stash);
		};
		
		struct Data: public SpecialTrackCollection<MediaLibraryTracksCollectionItem>::Data {
			Filters filters;
			
			static Data fromJson(const Json& json, MediaProviderStash* stash);
		};
		
		static String uri(const Filters& filters);
		static String name(const Filters& filters);
		static Data data(const Filters& filters, Optional<size_t> itemCount, Map<size_t,MediaLibraryTracksCollectionItem::Data> items);
		
		static $<MediaLibraryTracksCollection> new$(MediaLibraryProxyProvider* provider, const Filters& filters);
		MediaLibraryTracksCollection(MediaLibraryProxyProvider* provider, const Filters& filters);
		
		const Filters& filters() const;
		MediaDatabase* database();
		const MediaDatabase* database() const;
		
		virtual Promise<void> fetchData() override;
		void applyData(const Data& data);
		
		Data toData(const DataOptions& options = DataOptions()) const;
		virtual Json toJson(const ToJsonOptions& options) const override;
		
	protected:
		static $<MediaLibraryTracksCollection> new$(MediaLibraryProxyProvider* provider, const Data& data);
		MediaLibraryTracksCollection(MediaLibraryProxyProvider* provider, const Data& data);
		
		virtual MutatorDelegate* createMutatorDelegate() override final;
		
		virtual size_t getChunkSize() const override;
		virtual Promise<void> loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) override;
		virtual Promise<void> insertItems(Mutator* mutator, size_t index, LinkedList<$<Track>> tracks) override;
		virtual Promise<void> appendItems(Mutator* mutator, LinkedList<$<Track>> tracks) override;
		virtual Promise<void> removeItems(Mutator* mutator, size_t index, size_t count) override;
		virtual Promise<void> moveItems(Mutator* mutator, size_t index, size_t count, size_t newIndex) override;
		
		Filters _filters;
	};
}

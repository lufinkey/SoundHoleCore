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


	class MediaLibraryTracksCollectionItem: public SpecialTrackCollectionItem<MediaLibraryTracksCollection> {
	public:
		struct Data: public SpecialTrackCollectionItem<MediaLibraryTracksCollection>::Data {
			String addedAt;
		};
		
		static $<MediaLibraryTracksCollectionItem> new$($<SpecialTrackCollection<MediaLibraryTracksCollectionItem>> collection, const Data& data);
		
		MediaLibraryTracksCollectionItem($<SpecialTrackCollection<MediaLibraryTracksCollectionItem>> collection, const Data& data);
		MediaLibraryTracksCollectionItem($<SpecialTrackCollection<MediaLibraryTracksCollectionItem>> collection, const Json& json, MediaProviderStash* stash);
		
		const String& addedAt() const;
		
		virtual bool matchesItem(const TrackCollectionItem* item) const override;
		
		Data toData() const;
		
		static $<MediaLibraryTracksCollectionItem> fromJson($<SpecialTrackCollection<MediaLibraryTracksCollectionItem>> collection, const Json& json, MediaProviderStash* stash);
		virtual Json toJson() const override;
		
	protected:
		String _addedAt;
	};


	class MediaLibraryTracksCollection: public SpecialTrackCollection<MediaLibraryTracksCollectionItem>, public SpecialTrackCollection<MediaLibraryTracksCollectionItem>::MutatorDelegate {
	public:
		using LoadItemOptions = TrackCollection::LoadItemOptions;
		
		struct Filters {
			MediaProvider* libraryProvider = nullptr;
			sql::LibraryItemOrderBy orderBy = sql::LibraryItemOrderBy::ADDED_AT;
			sql::Order order = sql::Order::DESC;
		};
		
		struct Data: public SpecialTrackCollection<MediaLibraryTracksCollectionItem>::Data {
			Filters filters;
		};
		
		static $<MediaLibraryTracksCollection> new$(MediaDatabase* database, MediaProvider* provider, const Data& data);
		
		MediaLibraryTracksCollection(MediaDatabase* database, MediaProvider* provider, const Data& data);
		MediaLibraryTracksCollection(const Json& json, MediaDatabase* database);
		
		MediaProvider* libraryProvider();
		const MediaProvider* libraryProvider() const;
		String libraryProviderName() const;
		
		virtual Promise<void> fetchData() override;
		void applyData(const Data& data);
		
		Data toData(const DataOptions& options = DataOptions()) const;
		
		static $<MediaLibraryTracksCollection> fromJson(const Json& json, MediaDatabase* database);
		virtual Json toJson(const ToJsonOptions& options) const override;
		
	protected:
		virtual MutatorDelegate* createMutatorDelegate() override;
		
		virtual size_t getChunkSize() override;
		virtual Promise<void> loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) override;
		
		MediaDatabase* _database;
		Filters _filters;
	};
}

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

namespace sh {
	class MediaLibraryTracksCollection;


	class MediaLibraryTracksCollectionItem: public SpecialTrackCollectionItem<MediaLibraryTracksCollection> {
	public:
		struct Data: public SpecialTrackCollectionItem<MediaLibraryTracksCollection>::Data {
			String addedAt;
		};
		
		static $<MediaLibraryTracksCollectionItem> new$($<SpecialTrackCollection<MediaLibraryTracksCollectionItem>> collection, Data data);
		
		MediaLibraryTracksCollectionItem($<SpecialTrackCollection<MediaLibraryTracksCollectionItem>> collection, Data data);
		MediaLibraryTracksCollectionItem($<SpecialTrackCollection<MediaLibraryTracksCollectionItem>> collection, Json json, MediaProviderStash* stash);
		
		const String& addedAt() const;
		
		virtual bool matchesItem(const TrackCollectionItem* item) const override;
		
		Data toData() const;
		
		static $<MediaLibraryTracksCollectionItem> fromJson($<SpecialTrackCollection<MediaLibraryTracksCollectionItem>> collection, Json json, MediaProviderStash* stash);
		virtual Json toJson() const override;
		
	protected:
		String _addedAt;
	};


	class MediaLibraryTracksCollection: public SpecialTrackCollection<MediaLibraryTracksCollectionItem>, public SpecialTrackCollection<MediaLibraryTracksCollectionItem>::MutatorDelegate {
	public:
		using LoadItemOptions = TrackCollection::LoadItemOptions;
		
		struct Data: public SpecialTrackCollection<MediaLibraryTracksCollectionItem>::Data {
			String libraryProviderName;
		};
		
		static $<MediaLibraryTracksCollection> new$(MediaDatabase* database, MediaProvider* provider, Data data);
		
		MediaLibraryTracksCollection(MediaDatabase* database, MediaProvider* provider, Data data);
		MediaLibraryTracksCollection(Json json, MediaDatabase* database);
		
		const String& libraryProviderName() const;
		
		virtual Promise<void> fetchData() override;
		void applyData(const Data& data);
		
		Data toData(DataOptions options = DataOptions()) const;
		
		static $<MediaLibraryTracksCollection> fromJson(Json json, MediaDatabase* database);
		virtual Json toJson(ToJsonOptions options) const override;
		
	protected:
		virtual MutatorDelegate* createMutatorDelegate() override;
		
		virtual Promise<void> loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) override;
		
		MediaDatabase* _database;
		String _libraryProviderName;
	};
}

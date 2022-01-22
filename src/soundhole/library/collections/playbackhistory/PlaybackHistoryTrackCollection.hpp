//
//  PlaybackHistoryTrackCollection.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/4/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/PlaybackHistoryItem.hpp>
#include <soundhole/media/TrackCollection.hpp>
#include <soundhole/database/SQLOrder.hpp>

namespace sh {
	class PlaybackHistoryTrackCollection;
	class MediaLibraryProxyProvider;
	class MediaLibrary;

	class PlaybackHistoryTrackCollectionItem: public SpecialTrackCollectionItem<PlaybackHistoryTrackCollection> {
	public:
		struct Data: public SpecialTrackCollectionItem<PlaybackHistoryTrackCollection>::Data {
			$<PlaybackHistoryItem> historyItem;
			
			static Data forHistoryItem($<PlaybackHistoryItem> historyItem);
			static Data fromJson(const Json& json, MediaProviderStash* stash);
		};
		
		static $<PlaybackHistoryTrackCollectionItem> new$($<SpecialTrackCollection<PlaybackHistoryTrackCollectionItem>> collection, const Data& data);
		
		PlaybackHistoryTrackCollectionItem($<SpecialTrackCollection<PlaybackHistoryTrackCollectionItem>> collection, const Data& data);
		
		$<PlaybackHistoryItem> historyItem();
		$<const PlaybackHistoryItem> historyItem() const;
		
		virtual bool matchesItem(const TrackCollectionItem* item) const override;
		
		Data toData() const;
		virtual Json toJson() const override;
		
	protected:
		$<PlaybackHistoryItem> _historyItem;
	};


	class PlaybackHistoryTrackCollection: public SpecialTrackCollection<PlaybackHistoryTrackCollectionItem>, public SpecialTrackCollection<PlaybackHistoryTrackCollectionItem>::MutatorDelegate {
		friend class MediaLibraryProxyProvider;
	public:
		using LoadItemOptions = TrackCollection::LoadItemOptions;
		static auto constexpr TYPE = "playback history";
		
		struct Filters {
			MediaProvider* provider = nullptr;
			ArrayList<String> trackURIs;
			Optional<Date> minDate;
			bool minDateInclusive = true;
			Optional<Date> maxDate;
			bool maxDateInclusive = false;
			Optional<double> minDuration;
			Optional<double> minDurationRatio;
			Optional<bool> includeNullDuration;
			Optional<PlaybackHistoryItem::Visibility> visibility;
			ArrayList<String> scrobbledBy;
			ArrayList<String> notScrobbledBy;
			sql::Order order = sql::Order::DESC;
			
			static Filters fromJson(const Json& json, MediaProviderStash* stash);
			Json toJson() const;
		};
		
		struct Data: public SpecialTrackCollection<PlaybackHistoryTrackCollectionItem>::Data {
			Filters filters;
			
			static Data fromJson(const Json& json, MediaProviderStash* stash);
		};
		
		static String uri(const Filters& filters);
		static String name(const Filters& filters);
		static Data data(const Filters& filters, Optional<size_t> itemCount, Map<size_t,PlaybackHistoryTrackCollectionItem::Data> items);
		
		static $<PlaybackHistoryTrackCollection> new$(MediaLibraryProxyProvider* provider, const Filters& filters);
		static $<PlaybackHistoryTrackCollection> new$(MediaLibraryProxyProvider* provider, const Data& data);
		PlaybackHistoryTrackCollection(MediaLibraryProxyProvider* provider, const Filters& filters);
		PlaybackHistoryTrackCollection(MediaLibraryProxyProvider* provider, const Data& data);
		
		const Filters& filters() const;
		
		MediaLibrary* library();
		const MediaLibrary* library() const;
		
		MediaDatabase* database();
		const MediaDatabase* database() const;
		
		virtual Promise<void> fetchData() override;
		void applyData(const Data& data);
		
		Data toData(const DataOptions& options = DataOptions()) const;
		virtual Json toJson(const ToJsonOptions& options) const override;
		
		using SpecialTrackCollection<PlaybackHistoryTrackCollectionItem>::loadItems;
		
	protected:
		virtual MutatorDelegate* createMutatorDelegate() override final;
		
		virtual size_t getChunkSize() const override;
		virtual Promise<void> loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) override;
		
		Filters _filters;
	};
}

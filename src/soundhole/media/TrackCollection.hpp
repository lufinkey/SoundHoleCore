//
//  TrackCollection.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "MediaItem.hpp"

namespace sh {
	class TrackCollection;


	class TrackCollectionItem {
	public:
		virtual ~TrackCollectionItem() {};
		
		virtual $<Track> track() = 0;
		virtual $<const Track> track() const = 0;
		virtual $<TrackCollection> context() = 0;
		virtual $<const TrackCollection> context() const = 0;
		
		virtual Optional<size_t> indexInContext() const = 0;
		virtual bool matchesItem(const TrackCollectionItem* item) const = 0;
	};


	class TrackCollection: public MediaItem {
	public:
		using MediaItem::MediaItem;
		
		virtual $<TrackCollectionItem> itemAt(size_t index) = 0;
		virtual $<const TrackCollectionItem> itemAt(size_t index) const = 0;
		virtual Promise<$<TrackCollectionItem>> getItem(size_t index) = 0;
		
		virtual size_t itemCount() const = 0;
		
		virtual Promise<void> loadItems(size_t index, size_t count);
	};
}

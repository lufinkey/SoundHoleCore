//
//  MediaLibraryTracksCollection.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 6/17/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "MediaLibraryTracksCollection.hpp"
#include <soundhole/library/MediaLibraryProxyProvider.hpp>

namespace sh {

	#pragma mark MediaLibraryTracksCollectionItem::Data

	MediaLibraryTracksCollectionItem::Data MediaLibraryTracksCollectionItem::Data::fromJson(const Json& json, MediaProviderStash* stash) {
		return MediaLibraryTracksCollectionItem::Data{
			SpecialTrackCollectionItem<MediaLibraryTracksCollection>::Data::fromJson(json, stash),
			.addedAt = json["addedAt"].string_value()
		};
	}



	#pragma mark MediaLibraryTracksCollectionItem

	$<MediaLibraryTracksCollectionItem> MediaLibraryTracksCollectionItem::new$($<SpecialTrackCollection<MediaLibraryTracksCollectionItem>> context, const Data& data) {
		return fgl::new$<MediaLibraryTracksCollectionItem>(context, data);
	}

	MediaLibraryTracksCollectionItem::MediaLibraryTracksCollectionItem($<SpecialTrackCollection<MediaLibraryTracksCollectionItem>> context, const Data& data)
	: SpecialTrackCollectionItem<MediaLibraryTracksCollection>(context, data), _addedAt(data.addedAt) {
		//
	}

	const String& MediaLibraryTracksCollectionItem::addedAt() const {
		return _addedAt;
	}
	
	bool MediaLibraryTracksCollectionItem::matchesItem(const TrackCollectionItem* item) const {
		if(auto collectionItem = dynamic_cast<const MediaLibraryTracksCollectionItem*>(item)) {
			return (_track->uri() == collectionItem->_track->uri() && _addedAt == collectionItem->_addedAt);
		}
		return false;
	}

	MediaLibraryTracksCollectionItem::Data MediaLibraryTracksCollectionItem::toData() const {
		return {
			SpecialTrackCollectionItem<MediaLibraryTracksCollection>::toData(),
			.addedAt=_addedAt
		};
	}

	Json MediaLibraryTracksCollectionItem::toJson() const {
		auto json = SpecialTrackCollectionItem<MediaLibraryTracksCollection>::toJson().object_items();
		json.merge(Json::object{
			{ "addedAt", _addedAt.empty() ? Json() : Json(_addedAt) }
		});
		return json;
	}



	#pragma mark MediaLibraryTracksCollection::Data

	MediaLibraryTracksCollection::Data MediaLibraryTracksCollection::Data::fromJson(const Json& json, MediaProviderStash* stash) {
		return MediaLibraryTracksCollection::Data{
			SpecialTrackCollection<MediaLibraryTracksCollectionItem>::Data::fromJson(json,stash),
			.filters = Filters::fromJson(json["filters"], stash)
		};
	}

	#pragma mark MediaLibraryTracksCollection::Filters

	MediaLibraryTracksCollection::Filters MediaLibraryTracksCollection::Filters::fromJson(const Json& json, MediaProviderStash* stash) {
		auto filters = Filters();
		auto libraryProvider = json["libraryProvider"];
		if(libraryProvider.is_string()) {
			filters.libraryProvider = stash->getMediaProvider(libraryProvider.string_value());
		}
		auto orderBy = json["orderBy"];
		if(orderBy.is_string()) {
			filters.orderBy = sql::LibraryItemOrderBy_fromString(orderBy.string_value());
		}
		auto order = json["order"];
		if(order.is_string()) {
			filters.order = sql::Order_fromString(order.string_value());
		}
		return filters;
	}



	#pragma mark MediaLibraryTracksCollection static helpers

	String MediaLibraryTracksCollection::uri(const Filters& filters) {
		String uri = (String)MediaLibraryProxyProvider::NAME + ":collection:librarytracks";
		LinkedList<String> query;
		if(filters.libraryProvider != nullptr) {
			query.pushBack("libraryProvider="+filters.libraryProvider->name());
		}
		query.pushBack("orderBy="+sql::LibraryItemOrderBy_toString(filters.orderBy));
		query.pushBack("order="+sql::Order_toString(filters.order));
		return String::join({ uri,"?",String::join(query) });
	}

	String MediaLibraryTracksCollection::name(const Filters& filters) {
		return filters.libraryProvider ? ("My "+filters.libraryProvider->displayName()+" Tracks") : "My Tracks";
	}

	MediaLibraryTracksCollection::Data MediaLibraryTracksCollection::data(const Filters &filters, Optional<size_t> itemCount, Map<size_t,MediaLibraryTracksCollectionItem::Data> items) {
		return MediaLibraryTracksCollection::Data{{{
			.partial = false,
			.type = TYPE,
			.name = name(filters),
			.uri = uri(filters),
			.images = ArrayList<MediaItem::Image>{}
			},
			.versionId = String(),
			.itemCount = itemCount,
			.items = items,
			},
			.filters = filters
		};
	}



	#pragma mark MediaLibraryTracksCollection

	$<MediaLibraryTracksCollection> MediaLibraryTracksCollection::new$(MediaLibraryProxyProvider* provider, const Filters& filters) {
		auto collection = fgl::new$<MediaLibraryTracksCollection>(provider, filters);
		collection->lazyLoadContentIfNeeded();
		return collection;
	}
	
	MediaLibraryTracksCollection::MediaLibraryTracksCollection(MediaLibraryProxyProvider* provider, const Filters& filters)
	: MediaLibraryTracksCollection(provider, data(filters, std::nullopt, {})) {
		//
	}

	MediaLibraryTracksCollection::MediaLibraryTracksCollection(MediaLibraryProxyProvider* provider, const Data& data)
	: SpecialTrackCollection<MediaLibraryTracksCollectionItem>(provider, data),
	_filters(data.filters) {
		//
	}

	const MediaLibraryTracksCollection::Filters& MediaLibraryTracksCollection::filters() const {
		return _filters;
	}

	MediaDatabase* MediaLibraryTracksCollection::database() {
		return static_cast<MediaLibraryProxyProvider*>(mediaProvider())->database();
	}

	const MediaDatabase* MediaLibraryTracksCollection::database() const {
		return static_cast<const MediaLibraryProxyProvider*>(mediaProvider())->database();
	}

	Promise<void> MediaLibraryTracksCollection::fetchData() {
		auto self = std::static_pointer_cast<MediaLibraryTracksCollection>(shared_from_this());
		// TODO implement some sort of fetchData..?
		return Promise<void>::resolve();
	}

	void MediaLibraryTracksCollection::applyData(const Data& data) {
		SpecialTrackCollection<MediaLibraryTracksCollectionItem>::applyData(data);
		_filters = data.filters;
	}

	MediaLibraryTracksCollection::Data MediaLibraryTracksCollection::toData(const DataOptions& options) const {
		return {
			SpecialTrackCollection<MediaLibraryTracksCollectionItem>::toData(options),
			.filters=_filters
		};
	}

	Json MediaLibraryTracksCollection::toJson(const ToJsonOptions& options) const {
		auto json = SpecialTrackCollection<MediaLibraryTracksCollectionItem>::toJson(options).object_items();
		json.merge(Json::object{
			{ "filters", Json::object{
				{ "libraryProvider", (_filters.libraryProvider != nullptr) ? Json((std::string)_filters.libraryProvider->name()) : Json() },
				{ "orderBy", (std::string)sql::LibraryItemOrderBy_toString(_filters.orderBy) },
				{ "order", (std::string)sql::Order_toString(_filters.order) }
			} }
		});
		return json;
	}

	MediaLibraryTracksCollection::MutatorDelegate* MediaLibraryTracksCollection::createMutatorDelegate() {
		return this;
	}

	size_t MediaLibraryTracksCollection::getChunkSize() const {
		return 50;
	}

	Promise<void> MediaLibraryTracksCollection::loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) {
		auto self = std::static_pointer_cast<MediaLibraryTracksCollection>(shared_from_this());
		auto db = database();
		if(options.database != nullptr) {
			db = options.database;
		}
		return db->getSavedTracksJson(sql::IndexRange{
			.startIndex=index,
			.endIndex=(index+count)
		}, {
			.libraryProvider = (_filters.libraryProvider != nullptr) ? _filters.libraryProvider->name() : String(),
			.orderBy = _filters.orderBy,
			.order = _filters.order
		}).then([=](MediaDatabase::GetJsonItemsListResult results) {
			mutator->applyAndResize(index, results.total, results.items.map<$<MediaLibraryTracksCollectionItem>>([=](auto json) {
				auto jsonObj = json.object_items();
				auto trackJsonIt = jsonObj.find("mediaItem");
				if(trackJsonIt != jsonObj.end()) {
					auto trackJson = trackJsonIt->second;
					jsonObj.erase(trackJsonIt);
					jsonObj["track"] = trackJson;
				}
				return std::static_pointer_cast<MediaLibraryTracksCollectionItem>(
					self->createCollectionItem(jsonObj, db->getProviderStash()));
			}));
		});
	}

	Promise<void> MediaLibraryTracksCollection::insertItems(Mutator* mutator, size_t index, LinkedList<$<Track>> tracks) {
		return Promise<void>::reject(std::logic_error("Cannot insert items into MediaLibraryTracksCollection"));
	}

	Promise<void> MediaLibraryTracksCollection::appendItems(Mutator* mutator, LinkedList<$<Track>> tracks) {
		return Promise<void>::reject(std::logic_error("Cannot append items to MediaLibraryTracksCollection"));
	}

	Promise<void> MediaLibraryTracksCollection::removeItems(Mutator* mutator, size_t index, size_t count) {
		return Promise<void>::reject(std::logic_error("Not yet implemented"));
	}

	Promise<void> MediaLibraryTracksCollection::moveItems(Mutator* mutator, size_t index, size_t count, size_t newIndex) {
		return Promise<void>::reject(std::logic_error("Cannot move items in MediaLibraryTracksCollection"));
	}
}

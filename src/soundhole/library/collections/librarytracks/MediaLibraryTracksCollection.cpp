//
//  MediaLibraryTracksCollection.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 6/17/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "MediaLibraryTracksCollection.hpp"

namespace sh {
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

	$<MediaLibraryTracksCollectionItem> MediaLibraryTracksCollectionItem::fromJson($<SpecialTrackCollection<MediaLibraryTracksCollectionItem>> context, const Json& json, MediaProviderStash* stash) {
		return fgl::new$<MediaLibraryTracksCollectionItem>(context, json, stash);
	}

	MediaLibraryTracksCollectionItem::MediaLibraryTracksCollectionItem($<SpecialTrackCollection<MediaLibraryTracksCollectionItem>> context, const Json& json, MediaProviderStash* stash)
	: SpecialTrackCollectionItem<MediaLibraryTracksCollection>(context, json, stash) {
		_addedAt = json["addedAt"].string_value();
	}

	Json MediaLibraryTracksCollectionItem::toJson() const {
		auto json = SpecialTrackCollectionItem<MediaLibraryTracksCollection>::toJson().object_items();
		json.merge(Json::object{
			{ "addedAt", Json(_addedAt) }
		});
		return json;
	}



	$<MediaLibraryTracksCollection> MediaLibraryTracksCollection::new$(MediaDatabase* database, MediaProvider* provider, const Data& data) {
		auto collection = fgl::new$<MediaLibraryTracksCollection>(database, provider, data);
		collection->lazyLoadContentIfNeeded();
		return collection;
	}
	
	MediaLibraryTracksCollection::MediaLibraryTracksCollection(MediaDatabase* database, MediaProvider* provider, const Data& data)
	: SpecialTrackCollection<MediaLibraryTracksCollectionItem>(provider, data),
	_database(database),
	_filters(data.filters) {
		//
	}

	MediaProvider* MediaLibraryTracksCollection::libraryProvider() {
		return _filters.libraryProvider;
	}

	const MediaProvider* MediaLibraryTracksCollection::libraryProvider() const {
		return _filters.libraryProvider;
	}

	String MediaLibraryTracksCollection::libraryProviderName() const {
		return (_filters.libraryProvider != nullptr) ? _filters.libraryProvider->name() : "";
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

	$<MediaLibraryTracksCollection> MediaLibraryTracksCollection::fromJson(const Json& json, MediaDatabase* database) {
		auto collection = fgl::new$<MediaLibraryTracksCollection>(json, database);
		collection->lazyLoadContentIfNeeded();
		return collection;
	}

	MediaLibraryTracksCollection::MediaLibraryTracksCollection(const Json& json, MediaDatabase* database)
	: SpecialTrackCollection<MediaLibraryTracksCollectionItem>(json, database->getProviderStash()),
	_database(database) {
		auto stash = database->getProviderStash();
		auto libraryProvider = json["libraryProvider"];
		if(libraryProvider.is_string()) {
			_filters.libraryProvider = stash->getMediaProvider(libraryProvider.string_value());
		}
		auto orderBy = json["orderBy"];
		if(orderBy.is_string()) {
			_filters.orderBy = sql::LibraryItemOrderBy_fromString(orderBy.string_value());
		}
		auto order = json["order"];
		if(order.is_string()) {
			_filters.order = sql::Order_fromString(order.string_value());
		}
	}

	Json MediaLibraryTracksCollection::toJson(const ToJsonOptions& options) const {
		auto json = SpecialTrackCollection<MediaLibraryTracksCollectionItem>::toJson(options).object_items();
		json.merge(Json::object{
			{ "libraryProvider", (_filters.libraryProvider != nullptr) ? Json((std::string)_filters.libraryProvider->name()) : Json() },
			{ "orderBy", (std::string)sql::LibraryItemOrderBy_toString(_filters.orderBy) },
			{ "order", (std::string)sql::Order_toString(_filters.order) }
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
		MediaDatabase* database = _database;
		if(options.database != nullptr) {
			database = options.database;
		}
		return database->getSavedTracksJson(sql::IndexRange{
			.startIndex=index,
			.endIndex=(index+count)
		}, {
			.libraryProvider=(_filters.libraryProvider != nullptr) ? _filters.libraryProvider->name() : "",
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
				return MediaLibraryTracksCollectionItem::fromJson(self, jsonObj, database->getProviderStash());
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

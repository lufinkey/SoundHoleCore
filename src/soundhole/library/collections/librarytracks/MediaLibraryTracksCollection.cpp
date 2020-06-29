//
//  MediaLibraryTracksCollection.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 6/17/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "MediaLibraryTracksCollection.hpp"

namespace sh {
	$<MediaLibraryTracksCollectionItem> MediaLibraryTracksCollectionItem::new$($<SpecialTrackCollection<MediaLibraryTracksCollectionItem>> context, Data data) {
		return fgl::new$<MediaLibraryTracksCollectionItem>(context, data);
	}

	MediaLibraryTracksCollectionItem::MediaLibraryTracksCollectionItem($<SpecialTrackCollection<MediaLibraryTracksCollectionItem>> context, Data data)
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

	$<MediaLibraryTracksCollectionItem> MediaLibraryTracksCollectionItem::fromJson($<SpecialTrackCollection<MediaLibraryTracksCollectionItem>> context, Json json, MediaProviderStash* stash) {
		return fgl::new$<MediaLibraryTracksCollectionItem>(context, json, stash);
	}

	MediaLibraryTracksCollectionItem::MediaLibraryTracksCollectionItem($<SpecialTrackCollection<MediaLibraryTracksCollectionItem>> context, Json json, MediaProviderStash* stash)
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



	$<MediaLibraryTracksCollection> MediaLibraryTracksCollection::new$(MediaDatabase* database, MediaProvider* provider, Data data) {
		auto collection = fgl::new$<MediaLibraryTracksCollection>(database, provider, data);
		collection->lazyLoadContentIfNeeded();
		return collection;
	}
	
	MediaLibraryTracksCollection::MediaLibraryTracksCollection(MediaDatabase* database, MediaProvider* provider, Data data)
	: SpecialTrackCollection<MediaLibraryTracksCollectionItem>(provider, data),
	_database(database),
	_libraryProviderName(data.libraryProviderName) {
		//
	}

	const String& MediaLibraryTracksCollection::libraryProviderName() const {
		return _libraryProviderName;
	}

	Promise<void> MediaLibraryTracksCollection::fetchData() {
		auto self = std::static_pointer_cast<MediaLibraryTracksCollection>(shared_from_this());
		// TODO implement some sort of fetchData..?
		return Promise<void>::resolve();
	}

	void MediaLibraryTracksCollection::applyData(const Data& data) {
		SpecialTrackCollection<MediaLibraryTracksCollectionItem>::applyData(data);
	}

	MediaLibraryTracksCollection::Data MediaLibraryTracksCollection::toData(DataOptions options) const {
		return MediaLibraryTracksCollection::Data{
			SpecialTrackCollection<MediaLibraryTracksCollectionItem>::toData(options),
			.libraryProviderName=_libraryProviderName
		};
	}

	$<MediaLibraryTracksCollection> MediaLibraryTracksCollection::fromJson(Json json, MediaDatabase* database) {
		auto collection = fgl::new$<MediaLibraryTracksCollection>(json, database);
		collection->lazyLoadContentIfNeeded();
		return collection;
	}

	MediaLibraryTracksCollection::MediaLibraryTracksCollection(Json json, MediaDatabase* database)
	: SpecialTrackCollection<MediaLibraryTracksCollectionItem>(json, database->getProviderStash()),
	_database(database) {
		_libraryProviderName = json["libraryProviderName"].string_value();
	}

	Json MediaLibraryTracksCollection::toJson(ToJsonOptions options) const {
		auto json = SpecialTrackCollection<MediaLibraryTracksCollectionItem>::toJson(options).object_items();
		json.merge(Json::object{
			{ "libraryProviderName", Json(_libraryProviderName) }
		});
		return json;
	}

	MediaLibraryTracksCollection::MutatorDelegate* MediaLibraryTracksCollection::createMutatorDelegate() {
		return this;
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
			.libraryProvider=_libraryProviderName
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
}

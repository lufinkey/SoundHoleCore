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
		$<TrackCollectionItem> ptr;
		new MediaLibraryTracksCollectionItem(ptr, context, data);
		return std::static_pointer_cast<MediaLibraryTracksCollectionItem>(ptr);
	}

	MediaLibraryTracksCollectionItem::MediaLibraryTracksCollectionItem($<TrackCollectionItem>& ptr, $<SpecialTrackCollection<MediaLibraryTracksCollectionItem>> context, Data data)
	: SpecialTrackCollectionItem<MediaLibraryTracksCollection>(ptr, context, data), _addedAt(data.addedAt) {
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
		$<TrackCollectionItem> ptr;
		new MediaLibraryTracksCollectionItem(ptr, context, json, stash);
		return std::static_pointer_cast<MediaLibraryTracksCollectionItem>(ptr);
	}

	MediaLibraryTracksCollectionItem::MediaLibraryTracksCollectionItem($<TrackCollectionItem>& ptr, $<SpecialTrackCollection<MediaLibraryTracksCollectionItem>> context, Json json, MediaProviderStash* stash)
	: SpecialTrackCollectionItem<MediaLibraryTracksCollection>(ptr, context, json, stash) {
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
		$<MediaItem> ptr;
		new MediaLibraryTracksCollection(ptr, database, provider, data);
		return std::static_pointer_cast<MediaLibraryTracksCollection>(ptr);
	}
	
	MediaLibraryTracksCollection::MediaLibraryTracksCollection($<MediaItem>& ptr, MediaDatabase* database, MediaProvider* provider, Data data)
	: SpecialTrackCollection<MediaLibraryTracksCollectionItem>(ptr, provider, data),
	_database(database),
	_libraryProviderName(data.libraryProviderName) {
		//
	}

	const String& MediaLibraryTracksCollection::libraryProviderName() const {
		return _libraryProviderName;
	}

	Promise<void> MediaLibraryTracksCollection::fetchData() {
		auto self = selfAs<Playlist>();
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
		$<MediaItem> ptr;
		new MediaLibraryTracksCollection(ptr, json, database);
		return std::static_pointer_cast<MediaLibraryTracksCollection>(ptr);
	}

	MediaLibraryTracksCollection::MediaLibraryTracksCollection($<MediaItem>& ptr, Json json, MediaDatabase* database)
	: SpecialTrackCollection<MediaLibraryTracksCollectionItem>(ptr, json, database->getProviderStash()),
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
		auto self = selfAs<MediaLibraryTracksCollection>();
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
				return MediaLibraryTracksCollectionItem::fromJson(self, json, database->getProviderStash());
			}));
		});
	}
}

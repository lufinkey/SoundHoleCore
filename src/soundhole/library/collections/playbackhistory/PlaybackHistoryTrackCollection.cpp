//
//  PlaybackHistoryTrackCollection.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/4/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "PlaybackHistoryTrackCollection.hpp"
#include <soundhole/library/MediaLibraryProxyProvider.hpp>
#include <soundhole/utils/HttpClient.hpp>

namespace sh {

	#pragma mark PlaybackHistoryTrackCollectionItem::Data

	PlaybackHistoryTrackCollectionItem::Data PlaybackHistoryTrackCollectionItem::Data::fromJson(const Json& json, MediaProviderStash* stash) {
		auto historyItem = PlaybackHistoryItem::fromJson(json, stash);
		return PlaybackHistoryTrackCollectionItem::Data{{
			.track = historyItem->track()
			},
			.historyItem = historyItem
		};
	}



	#pragma mark MediaLibraryTracksCollectionItem

	$<PlaybackHistoryTrackCollectionItem> PlaybackHistoryTrackCollectionItem::new$($<SpecialTrackCollection<PlaybackHistoryTrackCollectionItem>> context, const Data& data) {
		return fgl::new$<PlaybackHistoryTrackCollectionItem>(context, data);
	}

	PlaybackHistoryTrackCollectionItem::PlaybackHistoryTrackCollectionItem($<SpecialTrackCollection<PlaybackHistoryTrackCollectionItem>> context, const Data& data)
	: SpecialTrackCollectionItem<PlaybackHistoryTrackCollection>(context, data), _historyItem(data.historyItem) {
		//
	}

	$<PlaybackHistoryItem> PlaybackHistoryTrackCollectionItem::historyItem() {
		return _historyItem;
	}

	$<const PlaybackHistoryItem> PlaybackHistoryTrackCollectionItem::historyItem() const {
		return _historyItem;
	}
	
	bool PlaybackHistoryTrackCollectionItem::matchesItem(const TrackCollectionItem* item) const {
		if(auto cmpItem = dynamic_cast<const PlaybackHistoryTrackCollectionItem*>(item)) {
			return ((_historyItem->track()->uri() == cmpItem->_historyItem->track()->uri())
				&& (_historyItem->startTime() == cmpItem->_historyItem->startTime()));
		}
		return false;
	}

	PlaybackHistoryTrackCollectionItem::Data PlaybackHistoryTrackCollectionItem::toData() const {
		return {
			SpecialTrackCollectionItem<PlaybackHistoryTrackCollection>::toData(),
			.historyItem = _historyItem
		};
	}

	Json PlaybackHistoryTrackCollectionItem::toJson() const {
		return _historyItem->toJson();
	}



	#pragma mark PlaybackHistoryTrackCollection::Data

	PlaybackHistoryTrackCollection::Data PlaybackHistoryTrackCollection::Data::fromJson(const Json& json, MediaProviderStash* stash) {
		return PlaybackHistoryTrackCollection::Data{
			SpecialTrackCollection<PlaybackHistoryTrackCollectionItem>::Data::fromJson(json,stash),
			.filters = Filters::fromJson(json["filters"], stash)
		};
	}

	#pragma mark PlaybackHistoryTrackCollection::Filters

	PlaybackHistoryTrackCollection::Filters PlaybackHistoryTrackCollection::Filters::fromJson(const Json& json, MediaProviderStash* stash) {
		auto filters = Filters();
		// provider
		auto jsonVal = json["provider"];
		if(jsonVal.is_string()) {
			filters.provider = stash->getMediaProvider(jsonVal.string_value());
		}
		// trackURIs
		jsonVal = json["trackURIs"];
		if(jsonVal.is_array()) {
			filters.trackURIs = ArrayList<Json>(jsonVal.array_items()).map([](auto& json) -> String {
				return json.string_value();
			});
		}
		// minDate
		jsonVal = json["minDate"];
		if(jsonVal.is_string()) {
			filters.minDate = Date::maybeFromISOString(jsonVal.string_value());
		}
		// minDateInclusive
		jsonVal = json["minDateInclusive"];
		if(jsonVal.is_bool()) {
			filters.minDateInclusive = jsonVal.bool_value();
		}
		// maxDate
		jsonVal = json["maxDate"];
		if(jsonVal.is_string()) {
			filters.maxDate = Date::maybeFromISOString(jsonVal.string_value());
		}
		// maxDateInclusive
		jsonVal = json["maxDateInclusive"];
		if(jsonVal.is_bool()) {
			filters.maxDateInclusive = jsonVal.bool_value();
		}
		// minDuration
		jsonVal = json["minDuration"];
		if(jsonVal.is_number()) {
			filters.minDuration = jsonVal.number_value();
		}
		// minDurationRatio
		jsonVal = json["minDurationRatio"];
		if(jsonVal.is_number()) {
			filters.minDurationRatio = jsonVal.number_value();
		}
		// includeNullDuration
		jsonVal = json["includeNullDuration"];
		if(jsonVal.is_bool()) {
			filters.includeNullDuration = jsonVal.bool_value();
		}
		// order
		jsonVal = json["order"];
		if(jsonVal.is_string()) {
			filters.order = sql::Order_fromString(jsonVal.string_value());
		}
		return filters;
	}

	Json PlaybackHistoryTrackCollection::Filters::toJson() const {
		return Json::object{
			{ "provider", provider ? (std::string)provider->name() : Json() },
			{ "trackURIs", (!trackURIs.empty()) ? trackURIs.map([](auto& uri) {
				return Json((std::string)uri);
			}) : Json() },
			{ "minDate", minDate ? (std::string)minDate->toISOString() : Json() },
			{ "minDateInclusive", minDate ? minDateInclusive : Json() },
			{ "maxDate", maxDate ? (std::string)maxDate->toISOString() : Json() },
			{ "maxDateInclusive", maxDate ? maxDateInclusive : Json() },
			{ "minDuration", minDuration ? minDuration.value() : Json() },
			{ "minDurationRatio", minDurationRatio ? minDurationRatio.value() : Json() },
			{ "includeNullDuration", includeNullDuration.hasValue() ? includeNullDuration.value() : Json() },
			{ "order", sql::Order_toString(order) }
		};
	}



	#pragma mark PlaybackHistoryTrackCollection static helpers

	String PlaybackHistoryTrackCollection::uri(const Filters& filters) {
		Map<String,String> query;
		if(filters.provider != nullptr) {
			query["provider"] = filters.provider->name();
		}
		if(!filters.trackURIs.empty()) {
			query["trackURIs"] = String::join(filters.trackURIs, ",");
		}
		if(filters.minDate) {
			query["minDate"] = filters.minDate->toISOString();
			query["minDateInclusive"] = filters.minDateInclusive ? "true" : "false";
		}
		if(filters.maxDate) {
			query["maxDate"] = filters.maxDate->toISOString();
			query["maxDateInclusive"] = filters.maxDateInclusive ? "true" : "false";
		}
		if(filters.minDuration) {
			query["minDuration"] = std::to_string(filters.minDuration.value());
		}
		if(filters.minDurationRatio) {
			query["minDurationRatio"] = std::to_string(filters.minDurationRatio.value());
		}
		if(filters.includeNullDuration.hasValue()) {
			query["includeNullDuration"] = filters.includeNullDuration.value() ? "true" : "false";
		}
		query["order"] = sql::Order_toString(filters.order);
		return String::join({
			MediaLibraryProxyProvider::NAME,":collection:playbackhistory","?",utils::makeQueryString(query)
		});
	}

	String PlaybackHistoryTrackCollection::name(const Filters& filters) {
		return "Playback History";
	}

	PlaybackHistoryTrackCollection::Data PlaybackHistoryTrackCollection::data(const Filters &filters, Optional<size_t> itemCount, Map<size_t,PlaybackHistoryTrackCollectionItem::Data> items) {
		return PlaybackHistoryTrackCollection::Data{{{
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



	#pragma mark PlaybackHistoryTrackCollection

	$<PlaybackHistoryTrackCollection> PlaybackHistoryTrackCollection::new$(MediaLibraryProxyProvider* provider, const Filters& filters) {
		auto collection = fgl::new$<PlaybackHistoryTrackCollection>(provider, filters);
		collection->lazyLoadContentIfNeeded();
		return collection;
	}

	$<PlaybackHistoryTrackCollection> PlaybackHistoryTrackCollection::new$(MediaLibraryProxyProvider* provider, const Data& data) {
		auto collection = fgl::new$<PlaybackHistoryTrackCollection>(provider, data);
		collection->lazyLoadContentIfNeeded();
		return collection;
	}
	
	PlaybackHistoryTrackCollection::PlaybackHistoryTrackCollection(MediaLibraryProxyProvider* provider, const Filters& filters)
	: PlaybackHistoryTrackCollection(provider, data(filters, std::nullopt, {})) {
		//
	}

	PlaybackHistoryTrackCollection::PlaybackHistoryTrackCollection(MediaLibraryProxyProvider* provider, const Data& data)
	: SpecialTrackCollection<PlaybackHistoryTrackCollectionItem>(provider, data),
	_filters(data.filters) {
		//
	}

	const PlaybackHistoryTrackCollection::Filters& PlaybackHistoryTrackCollection::filters() const {
		return _filters;
	}

	MediaLibrary* PlaybackHistoryTrackCollection::library() {
		return static_cast<MediaLibraryProxyProvider*>(mediaProvider())->library();
	}

	const MediaLibrary* PlaybackHistoryTrackCollection::library() const {
		return static_cast<const MediaLibraryProxyProvider*>(mediaProvider())->library();
	}

	MediaDatabase* PlaybackHistoryTrackCollection::database() {
		return static_cast<MediaLibraryProxyProvider*>(mediaProvider())->database();
	}

	const MediaDatabase* PlaybackHistoryTrackCollection::database() const {
		return static_cast<const MediaLibraryProxyProvider*>(mediaProvider())->database();
	}

	Promise<void> PlaybackHistoryTrackCollection::fetchData() {
		auto self = std::static_pointer_cast<PlaybackHistoryTrackCollection>(shared_from_this());
		// TODO implement some sort of fetchData..?
		return resolveVoid();
	}

	void PlaybackHistoryTrackCollection::applyData(const Data& data) {
		SpecialTrackCollection<PlaybackHistoryTrackCollectionItem>::applyData(data);
		_filters = data.filters;
	}

	PlaybackHistoryTrackCollection::Data PlaybackHistoryTrackCollection::toData(const DataOptions& options) const {
		return {
			SpecialTrackCollection<PlaybackHistoryTrackCollectionItem>::toData(options),
			.filters=_filters
		};
	}

	Json PlaybackHistoryTrackCollection::toJson(const ToJsonOptions& options) const {
		auto json = SpecialTrackCollection<PlaybackHistoryTrackCollectionItem>::toJson(options).object_items();
		json.merge(Json::object{
			{ "filters", _filters.toJson() }
		});
		return json;
	}

	PlaybackHistoryTrackCollection::MutatorDelegate* PlaybackHistoryTrackCollection::createMutatorDelegate() {
		return this;
	}

	size_t PlaybackHistoryTrackCollection::getChunkSize() const {
		return 25;
	}

	Promise<void> PlaybackHistoryTrackCollection::loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) {
		auto self = _$(shared_from_this()).forceAs<PlaybackHistoryTrackCollection>();
		auto db = this->database();
		if(options.database != nullptr) {
			db = options.database;
		}
		return db->getPlaybackHistoryItemsJson({
			.filters = {
				.provider = _filters.provider ? _filters.provider->name() : String(),
				.trackURIs = _filters.trackURIs,
				.minDate = _filters.minDate,
				.minDateInclusive = _filters.minDateInclusive,
				.maxDate = _filters.maxDate,
				.maxDateInclusive = _filters.maxDateInclusive,
				.minDuration = _filters.minDuration,
				.minDurationRatio = _filters.minDurationRatio,
				.includeNullDuration = _filters.includeNullDuration
			},
			.range = sql::IndexRange{
				.startIndex = index,
				.endIndex = (count == (size_t)-1) ? (size_t)-1 : (index + count)
			},
			.order = _filters.order
		}).then([=](auto results) {
			mutator->applyAndResize(index, results.total, results.items.map([=](const Json& json) {
				return self->createCollectionItem(json, db->getProviderStash()).forceAs<PlaybackHistoryTrackCollectionItem>();
			}));
		});
	}
}

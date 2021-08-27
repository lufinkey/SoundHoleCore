//
//  TrackCollection.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "TrackCollection.hpp"
#include <soundhole/database/MediaDatabase.hpp>

namespace sh {

	#pragma mark TrackCollectionItem::Data

	TrackCollectionItem::Data TrackCollectionItem::Data::fromJson(const Json& json, MediaProviderStash* stash) {
		auto trackJson = json["track"];
		if(trackJson.is_null() || !trackJson.is_object()) {
			throw std::invalid_argument("track json cannot be null");
		}
		auto trackProviderName = trackJson["provider"].string_value();
		if(trackProviderName.empty()) {
			throw std::invalid_argument("track json is missing provider");
		}
		auto provider = stash->getMediaProvider(trackProviderName);
		if(provider == nullptr) {
			throw std::invalid_argument("invalid provider name for track: "+trackProviderName);
		}
		return TrackCollectionItem::Data{
			.track = provider->track(Track::Data::fromJson(trackJson, stash))
		};
	}



	#pragma mark TrackCollectionItem

	TrackCollectionItem::TrackCollectionItem($<TrackCollection> context, const Data& data)
	: _context(context), _track(data.track) {
		//
	}

	$<Track> TrackCollectionItem::track() {
		return _track;
	}

	$<const Track> TrackCollectionItem::track() const {
		return std::static_pointer_cast<const Track>(_track);
	}

	w$<TrackCollection> TrackCollectionItem::context() {
		return _context;
	}

	w$<const TrackCollection> TrackCollectionItem::context() const {
		return _context;
	}
	
	Optional<size_t> TrackCollectionItem::indexInContext() const {
		auto context = _context.lock();
		if(!context) {
			// TODO maybe fatal error?
			return std::nullopt;
		}
		return context->indexOfItemInstance(this);
	}

	void TrackCollectionItem::merge(const TrackCollectionItem* item) {
		_track->applyData(item->track()->toData());
	}

	TrackCollectionItem::Data TrackCollectionItem::toData() const {
		return TrackCollectionItem::Data{
			.track=_track
		};
	}

	Json TrackCollectionItem::toJson() const {
		return Json::object{
			{ "type", "collectionItem" },
			{ "track", _track->toJson() }
		};
	}



	#pragma mark TrackCollection::LoadItemOptions

	Map<String,Any> TrackCollection::LoadItemOptions::toMap() const {
		return {
			{ "database", database },
			{ "offline", offline },
			{ "forceReload", forceReload },
			{ "trackIndexChanes", trackIndexChanges }
		};
	}

	TrackCollection::LoadItemOptions TrackCollection::LoadItemOptions::fromMap(const Map<String,Any>& dict) {
		auto databaseIt = dict.find("database");
		auto offlineIt = dict.find("offline");
		auto forceReload = dict.find("forceReload");
		auto trackIndexChanges = dict.find("trackIndexChanges");
		return {
			.database = (databaseIt != dict.end()) ? databaseIt->second.maybeAs<MediaDatabase*>().valueOr(nullptr) : nullptr,
			.offline = (offlineIt != dict.end()) ? offlineIt->second.maybeAs<bool>().valueOr(false) : false,
			.forceReload = (forceReload != dict.end()) ? forceReload->second.maybeAs<bool>().valueOr(false) : false,
			.trackIndexChanges = (trackIndexChanges != dict.end()) ? trackIndexChanges->second.maybeAs<bool>().valueOr(false) : false
		};
	}

	TrackCollection::LoadItemOptions TrackCollection::LoadItemOptions::defaultOptions() {
		return LoadItemOptions();
	}



	#pragma mark TrackCollection

	String TrackCollection::versionId() const {
		return String();
	}

	Json TrackCollection::toJson() const {
		return toJson(ToJsonOptions());
	}

	Json TrackCollection::toJson(const ToJsonOptions& options) const {
		auto json = MediaItem::toJson().object_items();
		auto versionId = this->versionId();
		auto itemCount = this->itemCount();
		json.merge(Json::object{
			{ "versionId", versionId.empty() ? Json() : (std::string)versionId },
			{ "itemCount", itemCount ? (double)itemCount.value() : Json() }
		});
		return json;
	}



	#pragma mark TrackCollection::Subscriber

	TrackCollection::Subscriber::~Subscriber() {
		//
	}
}

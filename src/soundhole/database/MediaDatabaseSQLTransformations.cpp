//
//  MediaDatabaseSQLTransformations.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 7/7/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "MediaDatabaseSQLTransformations.hpp"

namespace sh::sql {

Json transformDBTrack(Json json) {
	auto obj = json.object_items();
	if(obj.find("type") == obj.end()) {
		obj["type"] = "track";
	}
	auto artistsJson = obj["artists"];
	if(artistsJson.is_string()) {
		std::string error;
		obj["artists"] = Json::parse(artistsJson.string_value(), error);
		if(!error.empty()) {
			throw std::invalid_argument("Failed to parse artists json: "+error);
		}
	}
	auto imagesJson = obj["images"];
	if(imagesJson.is_string()) {
		std::string error;
		obj["images"] = Json::parse(imagesJson.string_value(), error);
		if(!error.empty()) {
			throw std::invalid_argument("Failed to parse images json: "+error);
		}
	}
	return obj;
}

Json transformDBTrackCollection(Json collectionJson, Json ownerJson) {
	auto obj = collectionJson.object_items();
	if(!ownerJson["uri"].is_null()) {
		obj["owner"] = transformDBUserAccount(ownerJson);
	}
	auto artistsJson = obj["artists"];
	if(artistsJson.is_string()) {
		std::string error;
		obj["artists"] = Json::parse(artistsJson.string_value(), error);
		if(!error.empty()) {
			throw std::invalid_argument("Failed to parse artists json: "+error);
		}
	}
	auto imagesJson = obj["images"];
	if(imagesJson.is_string()) {
		std::string error;
		obj["images"] = Json::parse(imagesJson.string_value(), error);
		if(!error.empty()) {
			throw std::invalid_argument("Failed to parse images json: "+error);
		}
	}
	return obj;
}

Json transformDBTrackCollectionItem(Json collectionItemJson, Json trackJson) {
	auto obj = collectionItemJson.object_items();
	obj["track"] = transformDBTrack(trackJson);
	auto addedByJson = obj["addedBy"];
	if(addedByJson.is_string()) {
		std::string error;
		obj["addedBy"] = Json::parse(addedByJson.string_value(), error);
		if(!error.empty()) {
			throw std::invalid_argument("Failed to parse addedBy json: "+error);
		}
	}
	return obj;
}

Json combineDBTrackCollectionAndItems(Json collectionJson, LinkedList<Json> itemsJson) {
	auto obj = collectionJson.object_items();
	Json::object itemsJsonMap;
	for(auto itemJson : itemsJson) {
		auto indexNum = itemJson["indexNum"];
		if(!indexNum.is_number()) {
			continue;
		}
		itemsJsonMap[std::to_string((size_t)indexNum.number_value())] = itemJson;
	}
	obj["items"] = itemsJsonMap;
	return obj;
}

Json transformDBArtist(Json json) {
	auto obj = json.object_items();
	auto imagesJson = obj["images"];
	if(imagesJson.is_string()) {
		std::string error;
		obj["images"] = Json::parse(imagesJson.string_value(), error);
		if(!error.empty()) {
			throw std::invalid_argument("Failed to parse images json: "+error);
		}
	}
	return obj;
}

Json transformDBUserAccount(Json json) {
	auto obj = json.object_items();
	auto imagesJson = obj["images"];
	if(imagesJson.is_string()) {
		std::string error;
		obj["images"] = Json::parse(imagesJson.string_value(), error);
		if(!error.empty()) {
			throw std::invalid_argument("Failed to parse images json: "+error);
		}
	}
	return obj;
}

Json transformDBSavedTrack(Json savedTrackJson, Json trackJson) {
	auto obj = savedTrackJson.object_items();
	obj["mediaItem"] = transformDBTrack(trackJson);
	return obj;
}

Json transformDBSavedAlbum(Json savedAlbumJson, Json albumJson) {
	auto obj = savedAlbumJson.object_items();
	obj["mediaItem"] = transformDBTrackCollection(albumJson, nullptr);
	return obj;
}

Json transformDBSavedPlaylist(Json savedPlaylistJson, Json playlistJson, Json ownerJson) {
	auto obj = savedPlaylistJson.object_items();
	obj["mediaItem"] = transformDBTrackCollection(playlistJson, ownerJson);
	return obj;
}

Json transformDBFollowedArtist(Json followedArtistJson, Json artistJson) {
	auto obj = followedArtistJson.object_items();
	obj["mediaItem"] = transformDBArtist(artistJson);
	return obj;
}

Json transformDBFollowedUserAccount(Json followedUserAccountJson, Json userAccountJson) {
	auto obj = followedUserAccountJson.object_items();
	obj["mediaItem"] = transformDBUserAccount(userAccountJson);
	return obj;
}

Json transformDBPlaybackHistoryItem(Json historyItemJson, Json trackJson) {
	auto obj = historyItemJson.object_items();
	obj["track"] = transformDBTrack(trackJson);
	return obj;
}

Json transformDBScrobble(Json scrobbleJson) {
	auto obj = scrobbleJson.object_items();
	if(obj.contains("ignoredReason")) {
		auto ignoredReasonJson = obj["ignoredReason"];
		std::string parseError;
		obj["ignoredReason"] = ignoredReasonJson.is_null() ? Json() : Json::parse(ignoredReasonJson.string_value(), parseError);
	}
	return obj;
}

}

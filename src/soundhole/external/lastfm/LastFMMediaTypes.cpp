//
//  LastFMMediaTypes.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/5/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "LastFMMediaTypes.hpp"
#include "LastFMError.hpp"

namespace sh {
	std::map<String,String> LastFMScrobbleRequest::toQueryItems() const {
		std::map<String,String> queryItems;
		size_t i = 0;
		for(auto& item : items) {
			item.appendQueryItems(queryItems, i);
			i++;
		}
		return queryItems;
	}

	void LastFMScrobbleRequest::Item::appendQueryItems(std::map<String,String>& queryItems, size_t index) const {
		auto indexStr = std::to_string(index);
		queryItems["track["+indexStr+"]"] = track;
		queryItems["artist["+indexStr+"]"] = artist;
		queryItems["timestamp["+indexStr+"]"] = std::to_string((long long)timestamp.secondsSince1970());
		if(!album.empty()) {
			queryItems["album["+indexStr+"]"] = album;
		}
		if(!albumArtist.empty()) {
			queryItems["albumArtist["+indexStr+"]"] = albumArtist;
		}
		if(chosenByUser) {
			queryItems["chosenByUser["+indexStr+"]"] = chosenByUser.value() ? "1" : "0";
		}
		if(trackNumber) {
			queryItems["trackNumber["+indexStr+"]"] = std::to_string(trackNumber.value());
		}
		if(!mbid.empty()) {
			queryItems["mbid["+indexStr+"]"] = mbid;
		}
		if(duration) {
			queryItems["duration["+indexStr+"]"] = std::to_string(duration.value());
		}
	}


	LastFMScrobbleResponse LastFMScrobbleResponse::fromJson(const Json& json) {
		auto scrobblesJson = json["scrobbles"];
		auto attrJson = scrobblesJson["@attr"];
		auto acceptedJson = attrJson["accepted"];
		if(acceptedJson.is_null()) {
			throw LastFMError(LastFMError::Code::BAD_DATA, "missing required property 'scrobbles.@attr.accepted'");
		}
		auto ignoredJson = attrJson["ignored"];
		if(ignoredJson.is_null()) {
			throw LastFMError(LastFMError::Code::BAD_DATA, "missing required property 'scrobbles.@attr.ignored'");
		}
		auto scrobblesListJson = scrobblesJson["scrobble"];
		if(scrobblesListJson.is_null()) {
			throw LastFMError(LastFMError::Code::BAD_DATA, "missing required property 'scrobbles.scrobble'");
		}
		return LastFMScrobbleResponse{
			.accepted = acceptedJson.is_number() ? (size_t)acceptedJson.number_value() : std::stol(acceptedJson.string_value()),
			.ignored = ignoredJson.is_number() ? (size_t)ignoredJson.number_value() : std::stol(ignoredJson.string_value()),
			.scrobbles = scrobblesListJson.is_array() ? Item::arrayFromJson(scrobblesListJson) : ArrayList<Item>{ Item::fromJson(scrobblesListJson) }
		};
	}

	LastFMScrobbleResponse::Item LastFMScrobbleResponse::Item::fromJson(const Json& json) {
		auto timestampJson = json["timestamp"];
		return Item{
			.ignoredMessage = IgnoredMessage::fromJson(json["ignoredMessage"]),
			.track = Property::fromJson(json["track"]),
			.artist = Property::fromJson(json["artist"]),
			.album = Property::fromJson(json["album"]),
			.albumArtist = Property::fromJson(json["albumArtist"]),
			.timestamp =
				timestampJson.is_number() ?
					Date::fromSecondsSince1970(timestampJson.number_value())
				: timestampJson.is_string() ?
					(String(timestampJson.string_value()).containsWhere([](auto c) { return !std::isdigit(c, std::locale()) && c != '.'; })) ?
						Date::fromSecondsSince1970(std::stod(timestampJson.string_value()))
						: Date::parse(timestampJson.string_value())
					: throw std::invalid_argument("missing required property 'timestamp'")
		};
	}

	LastFMScrobbleResponse::Item::Property LastFMScrobbleResponse::Item::Property::fromJson(const Json& json) {
		auto textJson = json["#text"];
		auto correctedJson = json["corrected"];
		return Property{
			.text = textJson.is_string() ? maybe(textJson.string_value()) : std::nullopt,
			.corrected =
				correctedJson.is_string() ?
					correctedJson.string_value().empty() ? false
					: (correctedJson.string_value() == "0" || correctedJson.string_value() == "false") ? false
					: (correctedJson.string_value() == "1" || correctedJson.string_value() == "true") ? true
					: (std::stoi(correctedJson.string_value()) != 0)
				: correctedJson.is_bool() ? correctedJson.bool_value()
				: correctedJson.is_number() ? (correctedJson.number_value() != 0)
				: false
		};
	}

	LastFMScrobbleResponse::IgnoredMessage LastFMScrobbleResponse::IgnoredMessage::fromJson(const Json& json) {
		auto textJson = json["#text"];
		auto codeJson = json["code"];
		return IgnoredMessage{
			.code =
				codeJson.is_string() ? codeJson.string_value()
				: codeJson.is_number() ?
					floatIsIntegral(codeJson.number_value()) ?
						std::to_string(numeric_cast<long long>(codeJson.number_value()))
						: std::to_string(codeJson.number_value())
				: String(),
			.text = textJson.string_value()
		};
	}
}

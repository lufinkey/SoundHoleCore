//
//  LastFMTypes.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/5/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "LastFMTypes.hpp"
#include "LastFMError.hpp"
#include <soundhole/utils/js/JSUtils.hpp>

namespace sh {
	#pragma mark Session

	LastFMSession LastFMSession::fromJson(const Json& json) {
		auto keyJson = json["key"];
		if(keyJson.string_value().empty()) {
			throw LastFMError(LastFMError::Code::BAD_DATA, "Missing required property 'key'");
		}
		return LastFMSession{
			.name = json["name"].string_value(),
			.subscriber = jsutils::badlyFormattedBoolFromJson(json["subscriber"]).valueOr(false),
			.key = keyJson.string_value()
		};
	}

	Json LastFMSession::toJson() const {
		return Json::object{
			{ "name", (std::string)name },
			{ "subscriber", subscriber },
			{ "key", (std::string)key }
		};
	}



	#pragma mark ScrobbleRequest

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
		if(duration) {
			queryItems["duration["+indexStr+"]"] = std::to_string(duration.value());
		}
		if(trackNumber) {
			queryItems["trackNumber["+indexStr+"]"] = std::to_string(trackNumber.value());
		}
		if(!mbid.empty()) {
			queryItems["mbid["+indexStr+"]"] = mbid;
		}
	}



	#pragma mark ScrobbleResponse

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
			.scrobbles = jsutils::singleOrArrayListFromJson(scrobblesListJson, [&](auto& itemJson) {
				return Item::fromJson(itemJson);
			})
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
						Date::parse(timestampJson.string_value())
						: Date::fromSecondsSince1970(std::stod(timestampJson.string_value()))
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



	#pragma mark Image

    LastFMImage LastFMImage::fromJson(const Json& json) {
		return LastFMImage{
			.url = json["#text"].string_value(),
			.size = json["size"].string_value()
		};
	}

	Json LastFMImage::toJson() const {
		return Json::object{
			{ "#text", (std::string)url },
			{ "size", (std::string)size }
		};
	}

	#pragma mark Tag

	LastFMTag LastFMTag::fromJson(const Json& json) {
		return LastFMTag{
			.name = json["name"].string_value(),
			.url = json["url"].string_value()
		};
	}

	#pragma mark Link

	LastFMLink LastFMLink::fromJson(const Json& json) {
		return LastFMLink{
			.text = json["#text"].string_value(),
			.rel = json["rel"].string_value(),
			.href = json["href"].string_value()
		};
	}



	#pragma mark PartialArtistInfo

	LastFMPartialArtistInfo LastFMPartialArtistInfo::fromJson(const Json& json) {
		if(json.is_string()) {
			return LastFMPartialArtistInfo{
				.name = json.string_value(),
				.url = "https://www.last.fm/music/"+URL::encodeQueryValue(json.string_value())
			};
		}
		return LastFMPartialArtistInfo{
			.name = json["name"].string_value(),
			.mbid = json["mbid"].string_value(),
			.url = json["url"].string_value(),
			.image = jsutils::optSingleOrArrayListFromJson(json["image"], [](auto& imageJson) {
				return LastFMImage::fromJson(imageJson);
			}),
			.listeners = jsutils::badlyFormattedSizeFromJson(json["listeners"]),
			.streamable = jsutils::badlyFormattedBoolFromJson(json["streamable"])
		};
	}



	#pragma mark UserInfo

    LastFMUserInfo LastFMUserInfo::fromJson(const Json& json) {
		return LastFMUserInfo{
			.type = json["type"].string_value(),
			.url = json["url"].string_value(),
			.name = json["name"].string_value(),
			.realname = json["realname"].string_value(),
			.country = json["country"].string_value(),
			.age = jsutils::stringFromJson(json["age"]),
			.gender = json["gender"].string_value(),
			.playCount = jsutils::badlyFormattedSizeFromJson(json["playcount"]),
			.subscriber = jsutils::badlyFormattedBoolFromJson(json["subscriber"]),
			.playlists = jsutils::stringFromJson(json["playlists"]),
			.bootstrap = jsutils::stringFromJson(json["bootstrap"]),
			.image = jsutils::singleOrArrayListFromJson(json["image"], [](const Json& imageJson) {
				return LastFMImage::fromJson(imageJson);
			}),
			.registeredTime = jsutils::stringFromJson(json["registered"]["#text"]),
			.registeredUnixTime = jsutils::stringFromJson(json["registered"]["unixtime"])
		};
    }

	Json LastFMUserInfo::toJson() const {
		auto json = Json::object{
			{ "type", (std::string)type },
			{ "url", (std::string)url },
			{ "name", (std::string)name },
			{ "realname", (std::string)realname },
			{ "country", (std::string)country },
			{ "age", (std::string)age },
			{ "gender", (std::string)gender },
			{ "playcount", playCount.hasValue() ? Json((double)playCount.value()) : Json() },
			{ "subscriber", subscriber.hasValue() ? Json(subscriber.value()) : Json() },
			{ "playlists", (std::string)playlists },
			{ "bootstrap", (std::string)bootstrap },
			{ "image", Json(image.map([](auto& img) {
				return img.toJson();
			})) }
		};
		auto registered = Json::object{};
		if(!registeredTime.empty()) {
			registered["#text"] = (std::string)registeredTime;
		}
		if(!registeredUnixTime.empty()) {
			registered["unixtime"] = (std::string)registeredUnixTime;
		}
		json["registered"] = registered;
		return json;
	}

	#pragma mark ArtistInfo

	LastFMArtistInfo LastFMArtistInfo::fromJson(const Json& json) {
		auto statsJson = json["stats"];
		return LastFMArtistInfo{
			.mbid = json["mbid"].string_value(),
			.url = json["url"].string_value(),
			.name = json["name"].string_value(),
			.image = jsutils::singleOrArrayListFromJson(json["image"], [](const Json& imageJson) {
				return LastFMImage::fromJson(imageJson);
			}),
			.streamable = jsutils::badlyFormattedBoolFromJson(json["streamable"]),
			.onTour = jsutils::badlyFormattedBoolFromJson(json["ontour"]),
			.stats = Stats{
				.listeners = jsutils::badlyFormattedSizeFromJson(statsJson["listeners"]),
				.playCount = jsutils::badlyFormattedSizeFromJson(statsJson["playcount"]),
				.userPlayCount = jsutils::badlyFormattedSizeFromJson(statsJson["userplaycount"])
			},
			.similarArtists = jsutils::singleOrArrayListFromJson(json["similar"]["artist"], [](const Json& artistJson) {
				return LastFMPartialArtistInfo::fromJson(artistJson);
			}),
			.tags = jsutils::singleOrArrayListFromJson(json["tags"]["tag"], [](const Json& artistJson) {
				return LastFMTag::fromJson(artistJson);
			}),
			.bio = Bio{
				.published = DateFormatter{
					.format = "%d %b %Y, %H:%M",
					.timeZone = TimeZone::gmt()
				}.dateFromString(json["published"].string_value()),
				.summary = json["summary"].string_value(),
				.content = json["content"].string_value(),
				.links = jsutils::singleOrArrayListFromJson(json["links"]["link"], [](const Json& linkJson) {
					return LastFMLink::fromJson(linkJson);
				})
			}
		};
	}

	#pragma mark TrackInfo

    LastFMTrackInfo LastFMTrackInfo::fromJson(const Json& json) {
        auto streamableJson = json["streamable"];
        return LastFMTrackInfo{
            .name = json["name"].string_value(),
			.mbid = json["mbid"].string_value(),
            .url = json["url"].string_value(),
			.duration = ([&]() -> Optional<double> {
				auto val = jsutils::badlyFormattedDoubleFromJson(json["duration"]);
				if(val == 0.0) {
					return std::nullopt;
				}
				return val;
			})(),
            .isStreamable = jsutils::badlyFormattedBoolFromJson(streamableJson["#text"]),
			.isFullTrackStreamable = jsutils::badlyFormattedBoolFromJson(streamableJson["fulltrack"]),
			.listeners = jsutils::badlyFormattedSizeFromJson(json["listeners"]),
			.playCount = jsutils::badlyFormattedSizeFromJson(json["playcount"]),
			.userPlayCount = jsutils::badlyFormattedSizeFromJson(json["userplaycount"]),
			.userLoved = jsutils::badlyFormattedBoolFromJson(json["userloved"]),
			.artist = LastFMPartialArtistInfo::fromJson(json["artist"]),
			.album = Album::maybeFromJson(json["album"]),
			.topTags = jsutils::optSingleOrArrayListFromJson(json["toptags"]["tag"], [](const Json& tagJson) {
				return LastFMTag::fromJson(tagJson);
			}),
			.wiki = Wiki::maybeFromJson(json["wiki"])
        };
    }

	LastFMTrackInfo::Album LastFMTrackInfo::Album::fromJson(const Json& json) {
		return Album{
			.artist = json["artist"].string_value(),
			.title = json["title"].string_value(),
			.url = json["url"].string_value(),
			.image = jsutils::singleOrArrayListFromJson(json["image"], [](const Json& json) {
				return LastFMImage::fromJson(json);
			})
		};
	}

	Optional<LastFMTrackInfo::Album> LastFMTrackInfo::Album::maybeFromJson(const Json& json) {
		if(json.is_null()) {
			return std::nullopt;
		}
		return LastFMTrackInfo::Album::fromJson(json);
	}

	LastFMTrackInfo::Wiki LastFMTrackInfo::Wiki::fromJson(const Json& json) {
		return Wiki{
			.published = DateFormatter{
				.format = "%d %b %Y, %H:%M",
				.timeZone = TimeZone::gmt()
			}.dateFromString(json["published"].string_value()),
			.summary = json["summary"].string_value(),
			.content = json["content"].string_value()
		};
	}

	Optional<LastFMTrackInfo::Wiki> LastFMTrackInfo::Wiki::maybeFromJson(const Json& json) {
		if(json.is_null()) {
			return std::nullopt;
		}
		return LastFMTrackInfo::Wiki::fromJson(json);
	}



	#pragma mark AlbumInfo

	LastFMAlbumInfo LastFMAlbumInfo::fromJson(const Json& json) {
		return LastFMAlbumInfo{
			.mbid = json["mbid"].string_value(),
			.url = json["url"].string_value(),
			.name = json["name"].string_value(),
			.artist = json["artist"].string_value(),
			.tags = jsutils::singleOrArrayListFromJson(json["tags"]["tag"], [](const Json& tagJson) {
				return LastFMTag::fromJson(tagJson);
			}),
			.image = jsutils::singleOrArrayListFromJson(json["image"], [](const Json& imageJson) {
				return LastFMImage::fromJson(imageJson);
			}),
			.listeners = jsutils::badlyFormattedSizeFromJson(json["listeners"]),
			.playCount = jsutils::badlyFormattedSizeFromJson(json["playCount"]),
			.userPlayCount = jsutils::badlyFormattedSizeFromJson(json["userPlayCount"]),
			.tracks = jsutils::singleOrArrayListFromJson(json["tracks"]["track"], [](const Json& trackJson) {
				return Track::fromJson(trackJson);
			})
		};
	}

	LastFMAlbumInfo::Track LastFMAlbumInfo::Track::fromJson(const Json& json) {
		auto streamableJson = json["streamable"];
		return Track{
			.url = json["url"].string_value(),
			.name = json["name"].string_value(),
			.duration = ([&]() -> Optional<double> {
				auto val = jsutils::badlyFormattedDoubleFromJson(json["duration"]);
				if(val == 0.0) {
					return std::nullopt;
				}
				return val;
			})(),
			.rank = jsutils::badlyFormattedSizeFromJson(json["rank"]),
			.isStreamable = jsutils::badlyFormattedBoolFromJson(streamableJson["#text"]),
			.isFullTrackStreamable = jsutils::badlyFormattedBoolFromJson(streamableJson["fulltrack"]),
			.artist = LastFMPartialArtistInfo::fromJson(json["artist"])
		};
	}



	#pragma mark CorrectedArtist

	LastFMCorrectedArtist LastFMCorrectedArtist::fromJson(const Json& json) {
		return LastFMCorrectedArtist{
			.mbid = json["mbid"].string_value(),
			.name = json["name"].string_value(),
			.url = json["url"].string_value()
		};
	}

	#pragma mark CorrectedTrack

	LastFMCorrectedTrack LastFMCorrectedTrack::fromJson(const Json& json) {
		return LastFMCorrectedTrack{
			.mbid = json["mbid"].string_value(),
			.name = json["name"].string_value(),
			.url = json["url"].string_value(),
			.artist = LastFMCorrectedArtist::fromJson(json["artist"])
		};
	}

	#pragma mark TrackCorrectionResponse

	LastFMTrackCorrectionResponse LastFMTrackCorrectionResponse::fromJson(const Json& json) {
		ArrayList<Correction> corrections;
		auto correctionsJson = json["corrections"]["correction"];
		if(correctionsJson.is_array()) {
			corrections.reserve(correctionsJson.array_items().size());
			for(auto& correctionJson : correctionsJson.array_items()) {
				corrections.pushBack(Correction::fromJson(correctionJson));
			}
		} else if(correctionsJson.is_object()) {
			corrections.pushBack(Correction::fromJson(correctionsJson));
		}
		return LastFMTrackCorrectionResponse{
			.corrections = corrections
		};
	}

	LastFMTrackCorrectionResponse::Correction LastFMTrackCorrectionResponse::Correction::fromJson(const Json& json) {
		auto attrsJson = json["@attr"];
		return Correction{
			.track = LastFMCorrectedTrack::fromJson(json["track"]),
			.index = jsutils::badlyFormattedSizeFromJson(attrsJson["index"]),
			.artistCorrected = jsutils::badlyFormattedSizeFromJson(attrsJson["artistcorrected"]),
			.trackCorrected = jsutils::badlyFormattedSizeFromJson(attrsJson["trackcorrected"])
		};
	}

	#pragma mark ArtistCorrectionResponse

	LastFMArtistCorrectionResponse LastFMArtistCorrectionResponse::fromJson(const Json& json) {
		ArrayList<Correction> corrections;
		auto correctionsJson = json["corrections"]["correction"];
		if(correctionsJson.is_array()) {
			corrections.reserve(correctionsJson.array_items().size());
			for(auto& correctionJson : correctionsJson.array_items()) {
				corrections.pushBack(Correction::fromJson(correctionJson));
			}
		} else if(correctionsJson.is_object()) {
			corrections.pushBack(Correction::fromJson(correctionsJson));
		}
		return LastFMArtistCorrectionResponse{
			.corrections = corrections
		};
	}

	LastFMArtistCorrectionResponse::Correction LastFMArtistCorrectionResponse::Correction::fromJson(const Json& json) {
		auto attrsJson = json["@attr"];
		return Correction{
			.artist = LastFMCorrectedArtist::fromJson(json["artist"]),
			.index = jsutils::badlyFormattedSizeFromJson(attrsJson["index"])
		};
	}



	#pragma mark TrackSearchResults

	LastFMTrackSearchResults LastFMTrackSearchResults::fromJson(const Json& json) {
		return LastFMTrackSearchResults{
			.startPage =
				jsutils::badlyFormattedSizeFromJson(json["opensearch:Query"]["startPage"])
				.valueOrThrow(std::runtime_error("invalid value for 'opensearch:Query'.startPage")),
			.startIndex =
				jsutils::badlyFormattedSizeFromJson(json["opensearch:startIndex"])
				.valueOrThrow(std::runtime_error("invalid value for 'opensearch:startIndex'")),
			.itemsPerPage =
				jsutils::badlyFormattedSizeFromJson(json["opensearch:itemsPerPage"])
				.valueOrThrow(std::runtime_error("invalid value for 'opensearch:itemsPerPage'")),
			.totalResults =
				jsutils::badlyFormattedSizeFromJson(json["opensearch:totalResults"])
				.valueOrThrow(std::runtime_error("invalid value for 'opensearch.totalResults'")),
			.items = jsutils::singleOrArrayListFromJson(json["trackmatches"]["track"], [](const Json& itemJson) {
				return Item::fromJson(itemJson);
			})
		};
	}

	LastFMTrackSearchResults::Item LastFMTrackSearchResults::Item::fromJson(const Json& json) {
		return Item{
			.mbid = json["mbid"].string_value(),
			.name = json["name"].string_value(),
			.artist = json["artist"].string_value(),
			.url = json["url"].string_value(),
			.streamable = jsutils::badlyFormattedBoolFromJson(json["streamable"]),
			.listeners = jsutils::stringFromJson(json["listeners"]),
			.image = jsutils::singleOrArrayListFromJson(json["image"], [](const Json& imageJson) {
				return LastFMImage::fromJson(imageJson);
			})
		};
	}



	#pragma mark ArtistSearchResults

	LastFMArtistSearchResults LastFMArtistSearchResults::fromJson(const Json& json) {
		return LastFMArtistSearchResults{
			.startPage =
				jsutils::badlyFormattedSizeFromJson(json["opensearch:Query"]["startPage"])
				.valueOrThrow(std::runtime_error("invalid value for 'opensearch:Query'.startPage")),
			.startIndex =
				jsutils::badlyFormattedSizeFromJson(json["opensearch:startIndex"])
				.valueOrThrow(std::runtime_error("invalid value for 'opensearch:startIndex'")),
			.itemsPerPage =
				jsutils::badlyFormattedSizeFromJson(json["opensearch:itemsPerPage"])
				.valueOrThrow(std::runtime_error("invalid value for 'opensearch:itemsPerPage'")),
			.totalResults =
				jsutils::badlyFormattedSizeFromJson(json["opensearch:totalResults"])
				.valueOrThrow(std::runtime_error("invalid value for 'opensearch.totalResults'")),
			.items = jsutils::singleOrArrayListFromJson(json["artistmatches"]["artist"], [](const Json& itemJson) {
				return Item::fromJson(itemJson);
			})
		};
	}

	LastFMArtistSearchResults::Item LastFMArtistSearchResults::Item::fromJson(const Json& json) {
		return Item{
			.mbid = json["mbid"].string_value(),
			.name = json["name"].string_value(),
			.url = json["url"].string_value(),
			.streamable = jsutils::badlyFormattedBoolFromJson(json["streamable"]),
			.listeners = jsutils::stringFromJson(json["listeners"]),
			.image = jsutils::singleOrArrayListFromJson(json["image"], [](const Json& imageJson) {
				return LastFMImage::fromJson(imageJson);
			})
		};
	}



	#pragma mark AlbumSearchResults

	LastFMAlbumSearchResults LastFMAlbumSearchResults::fromJson(const Json& json) {
		return LastFMAlbumSearchResults{
			.startPage =
				jsutils::badlyFormattedSizeFromJson(json["opensearch:Query"]["startPage"])
				.valueOrThrow(std::runtime_error("invalid value for 'opensearch:Query'.startPage")),
			.startIndex =
				jsutils::badlyFormattedSizeFromJson(json["opensearch:startIndex"])
				.valueOrThrow(std::runtime_error("invalid value for 'opensearch:startIndex'")),
			.itemsPerPage =
				jsutils::badlyFormattedSizeFromJson(json["opensearch:itemsPerPage"])
				.valueOrThrow(std::runtime_error("invalid value for 'opensearch:itemsPerPage'")),
			.totalResults =
				jsutils::badlyFormattedSizeFromJson(json["opensearch:totalResults"])
				.valueOrThrow(std::runtime_error("invalid value for 'opensearch.totalResults'")),
			.items = jsutils::singleOrArrayListFromJson(json["albummatches"]["album"], [](const Json& itemJson) {
				return Item::fromJson(itemJson);
			}),
			.streamable = jsutils::badlyFormattedBoolFromJson(json["streamable"])
		};
	}

	LastFMAlbumSearchResults::Item LastFMAlbumSearchResults::Item::fromJson(const Json& json) {
		return Item{
			.mbid = json["mbid"].string_value(),
			.name = json["name"].string_value(),
			.artist = json["artist"].string_value(),
			.url = json["url"].string_value(),
			.image = jsutils::singleOrArrayListFromJson(json["image"], [](const Json& imageJson) {
				return LastFMImage::fromJson(imageJson);
			})
		};
	}



	#pragma mark ArtistTopAlbum

	LastFMArtistTopAlbum LastFMArtistTopAlbum::fromJson(const Json& json) {
		return LastFMArtistTopAlbum{
			.mbid = json["mbid"].string_value(),
			.name = json["name"].string_value(),
			.url = json["url"].string_value(),
			.artist = LastFMPartialArtistInfo::fromJson(json["artist"]),
			.image = jsutils::singleOrArrayListFromJson(json["image"], [](auto& imageJson) {
				return LastFMImage::fromJson(imageJson);
			}),
			.playCount = jsutils::badlyFormattedSizeFromJson(json["playcount"])
		};
	}

	LastFMArtistItemsPageAttrs LastFMArtistItemsPageAttrs::fromJson(const Json& json) {
		return LastFMArtistItemsPageAttrs{
			.artist = json["artist"].string_value(),
			.page = jsutils::badlyFormattedSizeFromJson(json["page"])
				.valueOrThrow(std::runtime_error("invalid valid for @attr.page")),
			.perPage = jsutils::badlyFormattedSizeFromJson(json["perPage"])
				.valueOrThrow(std::runtime_error("invalid value for @attr.perPage")),
			.totalPages = jsutils::badlyFormattedSizeFromJson(json["totalPages"])
				.valueOrThrow(std::runtime_error("invalid value for @attr.totalPages")),
			.total = jsutils::badlyFormattedSizeFromJson(json["total"])
				.valueOrThrow(std::runtime_error("invalid value for @attr.total"))
		};
	}



	#pragma mark UserRecentTrack

	LastFMUserRecentTrack LastFMUserRecentTrack::fromJson(const Json& json) {
		auto trackURL = json["url"].string_value();
		return LastFMUserRecentTrack{
			.mbid = json["mbid"].string_value(),
			.name = json["name"].string_value(),
			.url = trackURL,
			.image = jsutils::singleOrArrayListFromJson(json["image"], [](auto& imageJson) {
				return LastFMImage::fromJson(imageJson);
			}),
			.artist = ([&]() {
				auto artistJson = json["artist"];
				auto textJson = artistJson["#text"];
				auto artist = LastFMPartialArtistInfo::fromJson(artistJson);
				if(artistJson["name"].is_null() && !textJson.is_null() && artist.name.empty()) {
					artist.name = textJson.string_value();
				}
				if(artist.url.empty()) {
					if(!trackURL.empty()) {
						auto url = URL(trackURL);
						auto pathParts = url.pathParts();
						while(pathParts.size() > 2) {
							pathParts.popBack();
						}
						url.setPathParts(pathParts, false);
						artist.url = url.toString();
					}
					else if(!artist.name.empty()) {
						artist.url = "https://www.last.fm/music/"+URL::encodeQueryValue(artist.name);
					}
				}
				return artist;
			})(),
			.album = ([&]() -> Optional<Album> {
				auto albumJson = json["album"];
				if(albumJson.is_null()) {
					return std::nullopt;
				}
				auto album = Album{
					.mbid = albumJson["mbid"].string_value(),
					.name = albumJson["text"].string_value()
				};
				if(album.mbid.empty() && album.name.empty()) {
					return std::nullopt;
				}
				return album;
			})(),
			.streamable = jsutils::badlyFormattedBoolFromJson(json["streamable"]),
			.loved = jsutils::badlyFormattedBoolFromJson(json["loved"]),
			.nowPlaying = jsutils::badlyFormattedBoolFromJson(json["@attr"]["nowplaying"]),
			.date = ([&]() {
				auto dateJson = json["date"];
				auto unixTimestamp = jsutils::badlyFormattedDoubleFromJson(dateJson["uts"]);
				if(unixTimestamp.hasValue()) {
					return Date::fromSecondsSince1970(unixTimestamp.value());
				}
				return DateFormatter{
					.format = "%d %b %Y, %H:%M",
					.timeZone = TimeZone::gmt()
				}.dateFromString(dateJson["#text"].string_value())
					.valueOrThrow(std::invalid_argument("invalid date for recent track"));
			})()
		};
	}

	LastFMUserItemsPageAttrs LastFMUserItemsPageAttrs::fromJson(const Json& json) {
		return LastFMUserItemsPageAttrs{
			.user = json["user"].string_value(),
			.page = jsutils::badlyFormattedSizeFromJson(json["page"])
				.valueOrThrow(std::runtime_error("invalid valid for @attr.page")),
			.perPage = jsutils::badlyFormattedSizeFromJson(json["perPage"])
				.valueOrThrow(std::runtime_error("invalid value for @attr.perPage")),
			.totalPages = jsutils::badlyFormattedSizeFromJson(json["totalPages"])
				.valueOrThrow(std::runtime_error("invalid value for @attr.totalPages")),
			.total = jsutils::badlyFormattedSizeFromJson(json["total"])
				.valueOrThrow(std::runtime_error("invalid value for @attr.total"))
		};
	}
}

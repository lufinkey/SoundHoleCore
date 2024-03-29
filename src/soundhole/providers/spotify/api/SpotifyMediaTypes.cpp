//
//  SpotifyMediaTypes.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/23/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include <napi.h>
#include "SpotifyMediaTypes.hpp"
#include <soundhole/utils/js/JSWrapClass.hpp>

namespace sh {
	SpotifyCursor SpotifyCursor::fromJson(const Json& json) {
		return SpotifyCursor{
			.after = json["after"].string_value()
		};
	}


	SpotifySearchResults SpotifySearchResults::fromJson(const Json& json) {
		return SpotifySearchResults{
			.tracks = SpotifyPage<SpotifyTrack>::maybeFromJson(json["tracks"]),
			.albums = SpotifyPage<SpotifyAlbum>::maybeFromJson(json["albums"]),
			.artists = SpotifyPage<SpotifyArtist>::maybeFromJson(json["artists"]),
			.playlists = SpotifyPage<SpotifyPlaylist>::maybeFromJson(json["playlists"])
		};
	}


	Json SpotifyImage::toJson() const {
		return Json::object{
			{ "url", (std::string)url },
			{ "width", (int)width },
			{ "height", (int)height }
		};
	}

	SpotifyImage SpotifyImage::fromJson(const Json& json) {
		return SpotifyImage{
			.url = json["url"].string_value(),
			.width = (size_t)json["width"].int_value(),
			.height = (size_t)json["height"].int_value()
		};
	}


	SpotifyCopyright SpotifyCopyright::fromJson(const Json& json) {
		return SpotifyCopyright{
			.text = json["text"].string_value(),
			.type = json["type"].string_value()
		};
	}


	Json SpotifyFollowers::toJson() const {
		return Json::object{
			{ "href", (std::string)href },
			{ "total", (double)total }
		};
	}

	SpotifyFollowers SpotifyFollowers::fromJson(const Json& json) {
		return SpotifyFollowers{
			.href = json["href"].string_value(),
			.total = (size_t)json["total"].number_value()
		};
	}

	Optional<SpotifyFollowers> SpotifyFollowers::maybeFromJson(const Json& json) {
		if(json.is_null()) {
			return std::nullopt;
		}
		return fromJson(json);
	}


	Json SpotifyUser::toJson() const {
		auto json = Json::object{
			{ "type", (std::string)type },
			{ "id", (std::string)id },
			{ "uri", (std::string)uri },
			{ "href", (std::string)href },
			{ "external_urls", jsutils::jsonFromMap<String>(externalURLs, [&](const String& key, const String& value) {
				return Json((std::string)value);
			}) }
		};
		if(displayName) {
			json["display_name"] = (std::string)displayName.value();
		}
		if(images) {
			json["images"] = images->map([=](auto& image) -> Json {
				return image.toJson();
			});
		}
		if(followers) {
			json["followers"] = followers->toJson();
		}
		return json;
	}

	SpotifyUser SpotifyUser::fromJson(const Json& json) {
		return SpotifyUser{
			.type = json["type"].string_value(),
			.id = json["id"].string_value(),
			.uri = json["uri"].string_value(),
			.href = json["href"].string_value(),
			.displayName = ([&]() -> Optional<String> {
				auto displayName = json["display_name"];
				if(displayName.is_null()) {
					return std::nullopt;
				}
				return displayName.string_value();
			})(),
			.images = jsutils::optArrayListFromJson(json["images"], [&](auto& json) -> SpotifyImage {
				return SpotifyImage::fromJson(json);
			}),
			.externalURLs = jsutils::mapFromJson(json["external_urls"], [=](const std::string& key, const Json& value) -> String {
				return value.string_value();
			}),
			.followers = SpotifyFollowers::maybeFromJson(json["followers"])
		};
	}


	SpotifyArtist SpotifyArtist::fromJson(const Json& json) {
		return SpotifyArtist{
			.type = json["type"].string_value(),
			.id = json["id"].string_value(),
			.uri = json["uri"].string_value(),
			.href = json["href"].string_value(),
			.name = json["name"].string_value(),
			.externalURLs = jsutils::mapFromJson(json["external_urls"], [=](const std::string& key, const Json& value) -> String {
				return value.string_value();
			}),
			.images = jsutils::optArrayListFromJson(json["images"], [&](auto& json) -> SpotifyImage {
				return SpotifyImage::fromJson(json);
			}),
			.genres = ([&]() -> Optional<ArrayList<String>> {
				auto genresJson = json["genres"];
				if(genresJson.is_null()) {
					return std::nullopt;
				}
				ArrayList<String> genres;
				genres.reserve(genresJson.array_items().size());
				for(auto& genre : genresJson.array_items()) {
					genres.pushBack(genre.string_value());
				}
				return genres;
			})(),
			.followers = SpotifyFollowers::maybeFromJson(json["followers"]),
			.popularity = ([&]() -> Optional<size_t> {
				auto popularityJson = json["popularity"];
				if(popularityJson.is_null()) {
					return std::nullopt;
				}
				return (size_t)popularityJson.number_value();
			})()
		};
	}


	SpotifyAlbum SpotifyAlbum::fromJson(const Json& json) {
		return SpotifyAlbum{
			.type = json["type"].string_value(),
			.albumType = json["album_type"].string_value(),
			.id = json["id"].string_value(),
			.uri = json["uri"].string_value(),
			.href = json["href"].string_value(),
			.name = json["name"].string_value(),
			.artists = jsutils::arrayListFromJson(json["artists"], [&](auto& json) -> SpotifyArtist {
				return SpotifyArtist::fromJson(json);
			}),
			.images = jsutils::arrayListFromJson(json["images"], [&](auto& json) -> SpotifyImage {
				return SpotifyImage::fromJson(json);
			}),
			.externalURLs = jsutils::mapFromJson(json["external_urls"], [=](const std::string& key, const Json& value) -> String {
				return value.string_value();
			}),
			.availableMarkets = jsutils::optArrayListFromJson(json["available_markets"], [&](auto& json) -> String {
				return json.string_value();
			}),
			.copyrights = jsutils::optArrayListFromJson(json["copyrights"], [&](auto& json) -> SpotifyCopyright {
				return SpotifyCopyright::fromJson(json);
			}),
			.label = ([&]() -> Optional<String> {
				auto label = json["label"];
				if(label.is_null()) {
					return std::nullopt;
				}
				return label.string_value();
			})(),
			.releaseDate = json["release_date"].string_value(),
			.releaseDatePrecision = json["release_date_precision"].string_value(),
			.tracks = SpotifyPage<SpotifyTrack>::maybeFromJson(json["tracks"])
		};
	}


	SpotifyTrack SpotifyTrack::fromJson(const Json& json) {
		return SpotifyTrack{
			.type = json["type"].string_value(),
			.id = json["id"].string_value(),
			.uri = json["uri"].string_value(),
			.href = json["href"].string_value(),
			.name = json["name"].string_value(),
			.album = ([&]() -> Optional<SpotifyAlbum> {
				auto album = json["album"];
				if(album.is_null()) {
					return std::nullopt;
				}
				return SpotifyAlbum::fromJson(album);
			})(),
			.artists = jsutils::arrayListFromJson(json["artists"], [&](auto& json) -> SpotifyArtist {
				return SpotifyArtist::fromJson(json);
			}),
			.availableMarkets = jsutils::optArrayListFromJson(json["available_markets"], [&](auto& json) -> String {
				return json.string_value();
			}),
			.externalIds = jsutils::mapFromJson(json["external_ids"], [=](const std::string& key, const Json& value) -> String {
				return value.string_value();
			}),
			.externalURLs = jsutils::mapFromJson(json["external_urls"], [=](const std::string& key, const Json& value) -> String {
				return value.string_value();
			}),
			.previewURL = json["preview_url"].string_value(),
			.trackNumber = (size_t)json["track_number"].number_value(),
			.discNumber = (size_t)json["disc_number"].number_value(),
			.durationMs = (uint64_t)json["duration_ms"].number_value(),
			.popularity = (size_t)json["popularity"].number_value(),
			.isLocal = json["is_local"].bool_value(),
			.isExplicit = json["explicit"].bool_value(),
			.isPlayable = ([&]() -> Optional<bool> {
				auto playable = json["is_playable"];
				if(playable.is_null()) {
					return std::nullopt;
				}
				return playable.bool_value();
			})()
		};
	}


	SpotifyPlaylist SpotifyPlaylist::fromJson(const Json& json) {
		auto isPublic = json["public"];
		return SpotifyPlaylist{
			.type = json["type"].string_value(),
			.id = json["id"].string_value(),
			.uri = json["uri"].string_value(),
			.href = json["href"].string_value(),
			.name = json["name"].string_value(),
			.owner = SpotifyUser::fromJson(json["owner"]),
			.description = ([&]() -> Optional<String> {
				auto description = json["description"];
				if(description.is_null()) {
					return std::nullopt;
				}
				return description.string_value();
			})(),
			.snapshotId = json["snapshot_id"].string_value(),
			.images = jsutils::arrayListFromJson(json["images"], [&](auto& json) -> SpotifyImage {
				return SpotifyImage::fromJson(json);
			}),
			.tracks = SpotifyPage<SpotifyPlaylist::Item>::fromJson(json["tracks"]),
			.externalURLs = jsutils::mapFromJson(json["external_urls"], [=](const std::string& key, const Json& value) -> String {
				return value.string_value();
			}),
			.followers = SpotifyFollowers::maybeFromJson(json["followers"]),
			.isPublic = (isPublic.is_bool() ? maybe(isPublic.bool_value()) : std::nullopt),
			.isCollaborative = json["collaborative"].bool_value()
		};
	}

	SpotifyPlaylist::Item SpotifyPlaylist::Item::fromJson(const Json& json) {
		auto addedBy = json["added_by"];
		return Item{
			.addedAt = json["added_at"].string_value(),
			.addedBy = addedBy.is_object() ? maybe(SpotifyUser::fromJson(addedBy)) : std::nullopt,
			.track = SpotifyTrack::fromJson(json["track"])
		};
	}

	SpotifyPlaylist::AddResult SpotifyPlaylist::AddResult::fromJson(const Json& json) {
		return AddResult{
			.snapshotId = json["snapshot_id"].string_value()
		};
	}

	SpotifyPlaylist::MoveResult SpotifyPlaylist::MoveResult::fromJson(const Json& json) {
		return MoveResult{
			.snapshotId = json["snapshot_id"].string_value()
		};
	}

	SpotifyPlaylist::RemoveResult SpotifyPlaylist::RemoveResult::fromJson(const Json& json) {
		return RemoveResult{
			.snapshotId = json["snapshot_id"].string_value()
		};
	}


	SpotifySavedTrack SpotifySavedTrack::fromJson(const Json& json) {
		return SpotifySavedTrack{
			.addedAt = json["added_at"].string_value(),
			.track = SpotifyTrack::fromJson(json["track"])
		};
	}

	SpotifySavedAlbum SpotifySavedAlbum::fromJson(const Json& json) {
		return SpotifySavedAlbum{
			.addedAt = json["added_at"].string_value(),
			.album = SpotifyAlbum::fromJson(json["album"])
		};
	}
}

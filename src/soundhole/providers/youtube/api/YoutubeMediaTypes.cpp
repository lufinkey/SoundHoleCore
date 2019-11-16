//
//  YoutubeMediaTypes.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/29/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "YoutubeMediaTypes.hpp"
#include "YoutubeError.hpp"
#include <soundhole/utils/js/JSWrapClass.hpp>

namespace sh {
	YoutubeImage::Size YoutubeImage::Size_fromString(const std::string& str) {
		if(str == "default") {
			return Size::DEFAULT;
		} else if(str == "medium") {
			return Size::MEDIUM;
		} else if(str == "high") {
			return Size::HIGH;
		} else if(str == "standard") {
			return Size::STANDARD;
		} else if(str == "maxres") {
			return Size::MAXRES;
		} else {
			throw YoutubeError(YoutubeError::Code::BAD_DATA, "Invalid image size \"" + str + "\"");
		}
	}

	ArrayList<YoutubeImage> YoutubeImage::arrayFromJson(const Json& json) {
		ArrayList<YoutubeImage> images;
		auto& items = json.object_items();
		images.reserve(items.size());
		for(auto& pair : items) {
			auto& itemKey = pair.first;
			auto& itemJson = pair.second;
			images.pushBack(YoutubeImage{
				.url = itemJson["url"].string_value(),
				.size = Size_fromString(itemKey),
				.dimensions = ([&]() -> Optional<Dimensions> {
					auto widthJson = itemJson["width"];
					auto heightJson = itemJson["height"];
					if(widthJson.is_null() || heightJson.is_null()) {
						return std::nullopt;
					}
					return Dimensions{
						.width=(size_t)widthJson.number_value(),
						.height=(size_t)heightJson.number_value()
					};
				})()
			});
		}
		return images;
	}



	YoutubeSearchResult YoutubeSearchResult::fromJson(const Json& json) {
		return YoutubeSearchResult{
			.kind = json["kind"].string_value(),
			.etag = json["etag"].string_value(),
			.id = Id::fromJson(json["id"]),
			.snippet = Snippet::fromJson(json["snippet"])
		};
	}

	YoutubeSearchResult::Id YoutubeSearchResult::Id::fromJson(const Json& json) {
		auto videoIdJson = json["videoId"];
		auto channelIdJson = json["channelId"];
		auto playlistIdJson = json["playlistId"];
		return Id{
			.kind = json["kind"].string_value(),
			.videoId = videoIdJson.is_null() ? std::nullopt : Optional<String>(videoIdJson.string_value()),
			.channelId = channelIdJson.is_null() ? std::nullopt : Optional<String>(channelIdJson.string_value()),
			.playlistId = playlistIdJson.is_null() ? std::nullopt : Optional<String>(playlistIdJson.string_value())
		};
	}

	YoutubeSearchResult::Snippet YoutubeSearchResult::Snippet::fromJson(const Json& json) {
		return Snippet{
			.channelId = json["channelId"].string_value(),
			.channelTitle = json["channelTitle"].string_value(),
			.title = json["title"].string_value(),
			.description = json["description"].string_value(),
			.publishedAt = json["publishedAt"].string_value(),
			.thumbnails = YoutubeImage::arrayFromJson(json["thumbnails"]),
			.liveBroadcastContent = json["liveBroadcastContent"].string_value()
		};
	}

	Optional<YoutubeImage> YoutubeSearchResult::Snippet::thumbnail(YoutubeImage::Size size) const {
		for(auto& image : thumbnails) {
			if(image.size == size) {
				return image;
			}
		}
		return std::nullopt;
	}


	
	YoutubeVideo YoutubeVideo::fromJson(const Json& json) {
		return YoutubeVideo{
			.kind = json["kind"].string_value(),
			.etag = json["etag"].string_value(),
			.id = json["id"].string_value(),
			.snippet = Snippet::fromJson(json["snippet"])
		};
	}

	YoutubeVideo::Snippet YoutubeVideo::Snippet::fromJson(const Json& json) {
		return YoutubeVideo::Snippet{
			.channelId = json["channelId"].string_value(),
			.channelTitle = json["channelTitle"].string_value(),
			.title = json["title"].string_value(),
			.description = json["description"].string_value(),
			.publishedAt = json["publishedAt"].string_value(),
			.thumbnails = YoutubeImage::arrayFromJson(json["thumbnails"]),
			.tags = JSWrapClass::arrayListFromJson<String>(json["tags"], [](auto& json) {
				return json.string_value();
			}),
			.categoryId = json["categoryId"].string_value(),
			.liveBroadcastContent = json["liveBroadcastContent"].string_value(),
			.defaultLanguage = json["defaultLanguage"].string_value(),
			.localized = Localized::fromJson(json["localized"]),
			.defaultAudioLanguage = json["defaultAudioLanguage"].string_value()
		};
	}

	YoutubeVideo::Snippet::Localized YoutubeVideo::Snippet::Localized::fromJson(const Json& json) {
		return Localized{
			.title = json["title"].string_value(),
			.description = json["description"].string_value()
		};
	}

	Optional<YoutubeImage> YoutubeVideo::Snippet::thumbnail(YoutubeImage::Size size) const {
		for(auto& image : thumbnails) {
			if(image.size == size) {
				return image;
			}
		}
		return std::nullopt;
	}



	YoutubeChannel YoutubeChannel::fromJson(const Json& json) {
		return YoutubeChannel{
			.kind = json["kind"].string_value(),
			.etag = json["etag"].string_value(),
			.id = json["id"].string_value(),
			.snippet = Snippet::fromJson(json["snippet"])
		};
	}

	YoutubeChannel::Snippet YoutubeChannel::Snippet::fromJson(const Json& json) {
		return Snippet{
			.title = json["title"].string_value(),
			.description = json["description"].string_value(),
			.customURL = json["customUrl"].string_value(),
			.publishedAt = json["publishedAt"].string_value(),
			.thumbnails = YoutubeImage::arrayFromJson(json["thumbnails"]),
			.defaultLanguage = json["defaultLanguage"].string_value(),
			.localized = Localized::fromJson(json["localized"]),
			.country = json["country"].string_value()
		};
	}

	YoutubeChannel::Snippet::Localized YoutubeChannel::Snippet::Localized::fromJson(const Json& json) {
		return Localized{
			.title = json["title"].string_value(),
			.description = json["description"].string_value()
		};
	}

	Optional<YoutubeImage> YoutubeChannel::Snippet::thumbnail(YoutubeImage::Size size) const {
		for(auto& image : thumbnails) {
			if(image.size == size) {
				return image;
			}
		}
		return std::nullopt;
	}



	YoutubePlaylist YoutubePlaylist::fromJson(const Json& json) {
		return YoutubePlaylist{
			.kind = json["kind"].string_value(),
			.etag = json["etag"].string_value(),
			.id = json["id"].string_value(),
			.snippet = Snippet::fromJson(json["snippet"])
		};
	}

	YoutubePlaylist::Snippet YoutubePlaylist::Snippet::fromJson(const Json& json) {
		return Snippet{
			.channelId = json["channelId"].string_value(),
			.channelTitle = json["channelTitle"].string_value(),
			.title = json["title"].string_value(),
			.description = json["description"].string_value(),
			.publishedAt = json["publishedAt"].string_value(),
			.thumbnails = YoutubeImage::arrayFromJson(json["thumbnails"]),
			.tags = JSWrapClass::arrayListFromJson<String>(json["tags"], [](auto& json) {
				return json.string_value();
			}),
			.defaultLanguage = json["defaultLanguage"].string_value(),
			.localized = Localized::fromJson(json["localized"])
		};
	}

	YoutubePlaylist::Snippet::Localized YoutubePlaylist::Snippet::Localized::fromJson(const Json& json) {
		return Localized{
			.title = json["title"].string_value(),
			.description = json["description"].string_value()
		};
	}

	Optional<YoutubeImage> YoutubePlaylist::Snippet::thumbnail(YoutubeImage::Size size) const {
		for(auto& image : thumbnails) {
			if(image.size == size) {
				return image;
			}
		}
		return std::nullopt;
	}
}

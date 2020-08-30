//
//  YoutubeMediaTypes.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/29/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include <napi.h>
#include "YoutubeMediaTypes.hpp"
#include "YoutubeError.hpp"
#include <soundhole/utils/js/JSUtils.hpp>

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



	YoutubeLocalization YoutubeLocalization::fromJson(const Json& json) {
		return YoutubeLocalization{
			.title = json["title"].string_value(),
			.description = json["description"].string_value()
		};
	}

	Json YoutubeLocalization::toJson() const {
		return Json::object{
			{ "title", (std::string)title },
			{ "description", (std::string)description }
		};
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
			.tags = jsutils::arrayListFromJson<String>(json["tags"], [](auto& json) {
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
		auto snippetJson = json["snippet"];
		auto contentDetailsJson = json["contentDetails"];
		return YoutubeChannel{
			.kind = json["kind"].string_value(),
			.etag = json["etag"].string_value(),
			.id = json["id"].string_value(),
			.snippet = Snippet::fromJson(json["snippet"]),
			.contentDetails = ContentDetails::fromJson(json["contentDetails"])
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

	YoutubeChannel::ContentDetails YoutubeChannel::ContentDetails::fromJson(const Json& json) {
		return ContentDetails{
			.relatedPlaylists=RelatedPlaylists::fromJson(json["relatedPlaylists"])
		};
	}

	YoutubeChannel::ContentDetails::RelatedPlaylists YoutubeChannel::ContentDetails::RelatedPlaylists::fromJson(const Json& json) {
		return RelatedPlaylists{
			.likes = json["liked"].string_value(),
			.favorites = json["favorites"].string_value(),
			.uploads = json["uploads"].string_value(),
			.watchHistory = json["watchHistory"].string_value(),
			.watchLater = json["watchLater"].string_value()
		};
	}



	YoutubePlaylist YoutubePlaylist::fromJson(const Json& json) {
		auto localizationsJson = json["localizations"].object_items();
		auto localizations = std::map<String,YoutubeLocalization>();
		for(auto& pair : localizationsJson) {
			localizations[pair.first] = YoutubeLocalization::fromJson(pair.second);
		}
		return YoutubePlaylist{
			.kind = json["kind"].string_value(),
			.etag = json["etag"].string_value(),
			.id = json["id"].string_value(),
			.snippet = Snippet::fromJson(json["snippet"]),
			.status = Status::fromJson(json["status"]),
			.contentDetails = ContentDetails::fromJson(json["contentDetails"]),
			.player = Player::fromJson(json["player"]),
			.localizations = localizations
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
			.tags = jsutils::arrayListFromJson<String>(json["tags"], [](auto& json) {
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

	YoutubePlaylist::Status YoutubePlaylist::Status::fromJson(const Json& json) {
		return Status{
			.privacyStatus = json["privacyStatus"].string_value()
		};
	}

	YoutubePlaylist::ContentDetails YoutubePlaylist::ContentDetails::fromJson(const Json& json) {
		return ContentDetails{
			.itemCount = (size_t)json["itemCount"].number_value()
		};
	}

	YoutubePlaylist::Player YoutubePlaylist::Player::fromJson(const Json& json) {
		return Player{
			.embedHtml = json["embedHtml"].string_value()
		};
	}

	Optional<YoutubeLocalization> YoutubePlaylist::localization(const String& key) const {
		auto it = localizations.find(key);
		if(it == localizations.end()) {
			return std::nullopt;
		}
		return it->second;
	}



	YoutubePlaylistItem YoutubePlaylistItem::fromJson(const Json& json) {
		return YoutubePlaylistItem{
			.kind = json["kind"].string_value(),
			.etag = json["etag"].string_value(),
			.id = json["id"].string_value(),
			.snippet = Snippet::fromJson(json["snippet"]),
			.contentDetails = ContentDetails::fromJson(json["contentDetails"]),
			.status = Status::fromJson(json["status"])
		};
	}

	YoutubePlaylistItem::Snippet YoutubePlaylistItem::Snippet::fromJson(const Json& json) {
		return Snippet{
			.channelId = json["channelId"].string_value(),
			.channelTitle = json["channelTitle"].string_value(),
			.title = json["title"].string_value(),
			.description = json["description"].string_value(),
			.publishedAt = json["publishedAt"].string_value(),
			.thumbnails = YoutubeImage::arrayFromJson(json["thumbnails"]),
			.playlistId = json["playlistId"].string_value(),
			.position = (size_t)json["position"].number_value(),
			.resourceId = ResourceId::fromJson(json["resourceId"])
		};
	}

	YoutubePlaylistItem::Snippet::ResourceId YoutubePlaylistItem::Snippet::ResourceId::fromJson(const Json& json) {
		return ResourceId{
			.kind=json["kind"].string_value(),
			.videoId=json["videoId"].string_value()
		};
	}

	YoutubePlaylistItem::ContentDetails YoutubePlaylistItem::ContentDetails::fromJson(const Json& json) {
		return ContentDetails{
			.videoId = json["videoId"].string_value(),
			.startAt = json["startAt"].string_value(),
			.endAt = json["endAt"].string_value(),
			.note = json["note"].string_value(),
			.videoPublishedAt = json["videoPublishedAt"].string_value()
		};
	}

	YoutubePlaylistItem::Status YoutubePlaylistItem::Status::fromJson(const Json& json) {
		return Status{
			.privacyStatus = json["privacyStatus"].string_value()
		};
	}




	YoutubeVideoInfo YoutubeVideoInfo::fromNapiObject(Napi::Object obj) {
		return YoutubeVideoInfo{
			.thumbnailURL = jsutils::stringFromNapiValue(obj.Get("thumbnail_url")),
			.formats = jsutils::arrayListFromNapiValue<Format>(obj.Get("formats"), [](Napi::Value value) {
				return Format::fromNapiObject(value.As<Napi::Object>());
			}),
			.media = Media::maybeFromNapiObject(obj.Get("media").As<Napi::Object>()),
			.author = Author::maybeFromNapiObject(obj.Get("author").As<Napi::Object>()),
			.playerResponse = PlayerResponse::fromNapiObject(obj.Get("player_response").As<Napi::Object>())
		};
	}

	YoutubeVideoInfo::Image YoutubeVideoInfo::Image::fromNapiObject(Napi::Object obj) {
		return Image{
			.url = jsutils::stringFromNapiValue(obj.Get("url")),
			.width = (size_t)obj.Get("width").As<Napi::Number>().Int64Value(),
			.height = (size_t)obj.Get("height").As<Napi::Number>().Int64Value()
		};
	}

	YoutubeVideoInfo::Format YoutubeVideoInfo::Format::fromNapiObject(Napi::Object obj) {
		return Format{
			.itag = jsutils::stringFromNapiValue(obj.Get("itag")),
			.url = jsutils::stringFromNapiValue(obj.Get("url")),
			.mimeType = jsutils::stringFromNapiValue(obj.Get("mimeType")),
			.bitrate = jsutils::stringFromNapiValue(obj.Get("bitrate")),
			.audioBitrate = jsutils::optDoubleFromNapiValue(obj.Get("audioBitrate")),
			.width = jsutils::optSizeFromNapiValue(obj.Get("width")),
			.height = jsutils::optSizeFromNapiValue(obj.Get("height")),
			.container = jsutils::stringFromNapiValue(obj.Get("container")),
			.hasVideo = jsutils::boolFromNapiValue(obj.Get("hasVideo")),
			.hasAudio = jsutils::boolFromNapiValue(obj.Get("hasAudio")),
			.codecs = jsutils::stringFromNapiValue(obj.Get("codecs")),
			.audioCodec = jsutils::stringFromNapiValue(obj.Get("audioCodec")),
			.videoCodec = jsutils::stringFromNapiValue(obj.Get("videoCodec")),
			.quality = jsutils::stringFromNapiValue(obj.Get("quality")),
			.qualityLabel = jsutils::stringFromNapiValue(obj.Get("qualityLabel")),
			.audioQuality = jsutils::stringFromNapiValue(obj.Get("audioQuality")),
		};
	}

	YoutubeVideoInfo::PlayerResponse YoutubeVideoInfo::PlayerResponse::fromNapiObject(Napi::Object obj) {
		return PlayerResponse{
			.videoDetails = VideoDetails::fromNapiObject(obj.Get("videoDetails").As<Napi::Object>())
		};
	}
	
	Optional<YoutubeVideoInfo::PlayerResponse> YoutubeVideoInfo::PlayerResponse::maybeFromNapiObject(Napi::Object obj) {
		if(obj.IsEmpty() || obj.IsNull() || obj.IsUndefined()) {
			return std::nullopt;
		}
		return fromNapiObject(obj);
	}

	YoutubeVideoInfo::VideoDetails YoutubeVideoInfo::VideoDetails::fromNapiObject(Napi::Object obj) {
		return VideoDetails{
			.videoId = jsutils::stringFromNapiValue(obj.Get("videoId")),
			.title = jsutils::stringFromNapiValue(obj.Get("title")),
			.shortDescription = jsutils::stringFromNapiValue(obj.Get("shortDescription")),
			.lengthSeconds = jsutils::stringFromNapiValue(obj.Get("lengthSeconds")),
			.keywords = jsutils::arrayListFromNapiValue<String>(obj.Get("keywords"), [](Napi::Value obj) {
				return jsutils::stringFromNapiValue(obj);
			}),
			.channelId = jsutils::stringFromNapiValue(obj.Get("channelId")),
			.isCrawlable = obj.Get("isCrawlable").ToBoolean(),
			.thumbnail = Thumbnail::fromNapiObject(obj.Get("thumbnail").As<Napi::Object>()),
			.viewCount = jsutils::stringFromNapiValue(obj.Get("viewCount")),
			.author = jsutils::stringFromNapiValue(obj.Get("author")),
			.isLiveContent = obj.Get("isLiveContent").ToBoolean()
		};
	}

	Optional<YoutubeVideoInfo::VideoDetails> YoutubeVideoInfo::VideoDetails::maybeFromNapiObject(Napi::Object obj) {
		if(obj.IsEmpty() || obj.IsNull() || obj.IsUndefined()) {
			return std::nullopt;
		}
		return fromNapiObject(obj);
	}

	YoutubeVideoInfo::VideoDetails::Thumbnail YoutubeVideoInfo::VideoDetails::Thumbnail::fromNapiObject(Napi::Object obj) {
		return Thumbnail{
			.thumbnails=jsutils::arrayListFromNapiValue<YoutubeVideoInfo::Image>(obj.Get("thumbnails"), [](Napi::Value obj) {
				return YoutubeVideoInfo::Image::fromNapiObject(obj.As<Napi::Object>());
			})
		};
	}

	YoutubeVideoInfo::Media YoutubeVideoInfo::Media::fromNapiObject(Napi::Object obj) {
		return Media{
			.image = jsutils::stringFromNapiValue(obj.Get("image")),
			.categoryURL = jsutils::stringFromNapiValue(obj.Get("category_url")),
			.category = jsutils::stringFromNapiValue(obj.Get("category")),
			.song = jsutils::stringFromNapiValue(obj.Get("song")),
			.year = jsutils::optSizeFromNapiValue(obj.Get("year")),
			.artist = jsutils::stringFromNapiValue(obj.Get("artist")),
			.artistURL = jsutils::stringFromNapiValue(obj.Get("artist_url")),
			.game = jsutils::stringFromNapiValue(obj.Get("artist")),
			.gameURL = jsutils::stringFromNapiValue(obj.Get("game_url")),
			.licensedBy = jsutils::stringFromNapiValue(obj.Get("licensed_by"))
		};
	}

	Optional<YoutubeVideoInfo::Media> YoutubeVideoInfo::Media::maybeFromNapiObject(Napi::Object obj) {
		if(obj.IsEmpty() || obj.IsNull() || obj.IsUndefined()) {
			return std::nullopt;
		}
		return fromNapiObject(obj);
	}

	YoutubeVideoInfo::Author YoutubeVideoInfo::Author::fromNapiObject(Napi::Object obj) {
		return Author{
			.id = jsutils::stringFromNapiValue(obj.Get("id")),
			.name = jsutils::stringFromNapiValue(obj.Get("name")),
			.avatar = jsutils::stringFromNapiValue(obj.Get("avatar")),
			.verified = obj.Get("verified").ToBoolean(),
			.user = jsutils::stringFromNapiValue(obj.Get("user")),
			.channelURL = jsutils::stringFromNapiValue(obj.Get("channel_url")),
			.externalChannelURL = jsutils::stringFromNapiValue(obj.Get("external_channel_url")),
			.userURL = jsutils::stringFromNapiValue(obj.Get("user_url")),
			.subscriberCount = jsutils::sizeFromNapiValue(obj.Get("subscriber_count"))
		};
	}

	Optional<YoutubeVideoInfo::Author> YoutubeVideoInfo::Author::maybeFromNapiObject(Napi::Object obj) {
		if(obj.IsEmpty() || obj.IsNull() || obj.IsUndefined()) {
			return std::nullopt;
		}
		return fromNapiObject(obj);
	}

	

	ArrayList<YoutubeVideoInfo::Format> YoutubeVideoInfo::Format::filterFormats(const ArrayList<Format>& formats, MediaTypeFilter filter) {
		switch(filter) {
			case MediaTypeFilter::AUDIO_AND_VIDEO:
				return formats.where([](auto& format) {
					return format.hasAudio && format.hasVideo;
				});
			case MediaTypeFilter::VIDEO:
				return formats.where([](auto& format) {
					return format.hasVideo;
				});
			case MediaTypeFilter::VIDEO_ONLY:
				return formats.where([](auto& format) {
					return format.hasVideo && !format.hasAudio;
				});
			case MediaTypeFilter::AUDIO:
				return formats.where([](auto& format) {
					return format.hasAudio;
				});
			case MediaTypeFilter::AUDIO_ONLY:
				return formats.where([](auto& format) {
					return format.hasAudio && !format.hasVideo;
				});
		}
		throw std::runtime_error("invalid media type filter");
	}

	Optional<YoutubeVideoInfo::Format> YoutubeVideoInfo::chooseAudioFormat(ChooseAudioFormatOptions options) const {
		auto formats = Format::filterFormats(this->formats, Format::MediaTypeFilter::AUDIO);
		formats.sort([&](auto& a, auto& b) {
			if(a.bitrate.empty() && !b.bitrate.empty()) {
				return true;
			} else if(!a.bitrate.empty() && b.bitrate.empty()) {
				return false;
			}
			if(options.bitrate.has_value()) {
				if(a.audioBitrate.value() >= options.bitrate.value() && b.audioBitrate.value() >= options.bitrate.value()) {
					return a.audioBitrate.value() < b.audioBitrate.value();
				} else {
					return a.audioBitrate.value() > b.audioBitrate.value();
				}
			} else {
				return a.audioBitrate.value() > b.audioBitrate.value();
			}
		});
		if(formats.size() == 0) {
			return std::nullopt;
		}
		return formats[0];
	}
}

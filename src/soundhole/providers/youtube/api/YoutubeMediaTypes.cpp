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



	YoutubePlaylistItem YoutubePlaylistItem::fromJson(const Json& json) {
		return YoutubePlaylistItem{
			.kind = json["kind"].string_value(),
			.etag = json["etag"].string_value(),
			.id = json["id"].string_value(),
			.snippet = Snippet::fromJson(json["snippet"])
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




	YoutubeVideoInfo YoutubeVideoInfo::fromNapiObject(Napi::Object obj) {
		return YoutubeVideoInfo{
			.videoId = jsutils::stringFromNapiValue(obj.Get("video_id")),
			.thumbnailURL = jsutils::stringFromNapiValue(obj.Get("thumbnail_url")),
			.title = jsutils::stringFromNapiValue(obj.Get("title")),
			.formats = jsutils::arrayListFromNapiValue<Format>(obj.Get("formats"), [](Napi::Value value) {
				return Format::fromNapiObject(value.As<Napi::Object>());
			}),
			.published = obj.Get("published").As<Napi::Number>().DoubleValue(),
			.description = jsutils::stringFromNapiValue(obj.Get("description")),
			.media = Media::maybeFromNapiObject(obj.Get("media").As<Napi::Object>()),
			.author = Author::maybeFromNapiObject(obj.Get("author").As<Napi::Object>()),
			.playerResponse = PlayerResponse::maybeFromNapiObject(obj.Get("player_response").As<Napi::Object>())
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
			.projectionType = jsutils::stringFromNapiValue(obj.Get("projection_type")),
			.clen = jsutils::stringFromNapiValue(obj.Get("clen")),
			.init = jsutils::stringFromNapiValue(obj.Get("init")),
			.fps = jsutils::stringFromNapiValue(obj.Get("fps")),
			.index = jsutils::stringFromNapiValue(obj.Get("index")),
			
			.itag = jsutils::stringFromNapiValue(obj.Get("itag")),
			.url = jsutils::stringFromNapiValue(obj.Get("url")),
			.type = jsutils::stringFromNapiValue(obj.Get("type")),
			
			.quality = jsutils::stringFromNapiValue(obj.Get("quality")),
			.qualityLabel = jsutils::stringFromNapiValue(obj.Get("quality_label")),
			.size = jsutils::stringFromNapiValue(obj.Get("size")),
			
			.sp = jsutils::stringFromNapiValue(obj.Get("sp")),
			.s = jsutils::stringFromNapiValue(obj.Get("s")),
			.container = jsutils::stringFromNapiValue(obj.Get("container")),
			.resolution = jsutils::stringFromNapiValue(obj.Get("resolution")),
			.encoding = jsutils::stringFromNapiValue(obj.Get("encoding")),
			.profile = jsutils::stringFromNapiValue(obj.Get("profile")),
			.bitrate = jsutils::stringFromNapiValue(obj.Get("bitrate")),
			
			.audioEncoding = jsutils::stringFromNapiValue(obj.Get("audioEncoding")),
			.audioBitrate = jsutils::optSizeFromNapiValue(obj.Get("audioBitrate")),
			
			.live = obj.Get("live").ToBoolean(),
			.isHLS = obj.Get("isHLS").ToBoolean(),
			.isDashMPD = obj.Get("isDashMPD").ToBoolean()
		};
	}

	YoutubeVideoInfo::PlayerResponse YoutubeVideoInfo::PlayerResponse::fromNapiObject(Napi::Object obj) {
		return PlayerResponse{
			.videoDetails = VideoDetails::maybeFromNapiObject(obj.Get("videoDetails").As<Napi::Object>())
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
			.lengthSeconds = jsutils::stringFromNapiValue(obj.Get("lengthSeconds")),
			.keywords = jsutils::arrayListFromNapiValue<String>(obj.Get("keywords"), [](Napi::Value obj) {
				return jsutils::stringFromNapiValue(obj);
			}),
			.channelId = jsutils::stringFromNapiValue(obj.Get("channelId")),
			.isCrawlable = obj.Get("isCrawlable").ToBoolean(),
			.thumbnail = Thumbnail::maybeFromNapiObject(obj.Get("thumbnail").As<Napi::Object>()),
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

	YoutubeVideoInfo::Media YoutubeVideoInfo::Media::fromNapiObject(Napi::Object obj) {
		return Media{
			.categoryURL = jsutils::stringFromNapiValue(obj.Get("category_url")),
			.category = jsutils::stringFromNapiValue(obj.Get("category")),
			.song = jsutils::stringFromNapiValue(obj.Get("song")),
			.artistURL = jsutils::stringFromNapiValue(obj.Get("artist_url")),
			.artist = jsutils::stringFromNapiValue(obj.Get("artist")),
			.licensedToYoutubeBy = jsutils::stringFromNapiValue(obj.Get("licensed_to_youtube_by"))
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
			.userURL = jsutils::stringFromNapiValue(obj.Get("user_url"))
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
					return !format.bitrate.empty() && format.audioBitrate.has_value();
				});
			case MediaTypeFilter::VIDEO:
				return formats.where([](auto& format) {
					return !format.bitrate.empty();
				});
			case MediaTypeFilter::VIDEO_ONLY:
				return formats.where([](auto& format) {
					return !format.bitrate.empty() && !format.audioBitrate.has_value();
				});
			case MediaTypeFilter::AUDIO:
				return formats.where([](auto& format) {
					return format.audioBitrate.has_value();
				});
			case MediaTypeFilter::AUDIO_ONLY:
				return formats.where([](auto& format) {
					return format.audioBitrate.has_value() && format.bitrate.empty();
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

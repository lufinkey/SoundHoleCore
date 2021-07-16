//
//  BandcampMediaTypes.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/21/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include <napi.h>
#include "BandcampMediaTypes.hpp"
#include "BandcampError.hpp"
#include <soundhole/utils/js/JSUtils.hpp>

namespace sh {
	Json BandcampImage::toJson() const {
		auto json = Json::object{
			{ "url", (std::string)url },
			{ "size", (std::string)Size_toString(size) }
		};
		if(dimensions) {
			json["width"] = (int)dimensions->width;
			json["height"] = (int)dimensions->height;
		}
		return json;
	}

	BandcampImage BandcampImage::fromJson(const Json& json) {
		return BandcampImage{
			.url=json["url"].string_value(),
			.size=Size_fromString(json["size"].string_value()),
			.dimensions = ([&]() -> Optional<Dimensions> {
				auto width = json["width"];
				auto height = json["height"];
				if(width.is_null() || height.is_null()) {
					return std::nullopt;
				}
				return Dimensions{
					.width = (size_t)width.number_value(),
					.height = (size_t)height.number_value()
				};
			})()
		};
	}

	BandcampImage BandcampImage::fromNapiObject(Napi::Object image) {
		return BandcampImage{
			.url = image.Get("url").As<Napi::String>().Utf8Value(),
			.size = ([&]() -> Size {
				auto size = image.Get("size").As<Napi::String>().Utf8Value();
				if(size == "small") {
					return Size::SMALL;
				} else if(size == "medium") {
					return Size::MEDIUM;
				} else if(size == "large") {
					return Size::LARGE;
				}
				return Size::MEDIUM;
			})(),
			.dimensions = ([&]() -> Optional<Dimensions> {
				auto width = image.Get("width").As<Napi::Number>();
				auto height = image.Get("height").As<Napi::Number>();
				if(width.IsEmpty() || width.IsNull() || width.IsUndefined()
				   || height.IsEmpty() || height.IsNull() || height.IsUndefined()) {
					return std::nullopt;
				}
				return Dimensions{
					.width = (size_t)width.Int64Value(),
					.height = (size_t)height.Int64Value()
				};
			})()
		};
	}

	BandcampImage::Size BandcampImage::Size_fromString(const String& size) {
		if(size == "small") {
			return Size::SMALL;
		} else if(size == "medium") {
			return Size::MEDIUM;
		} else if(size == "large") {
			return Size::LARGE;
		}
		return Size::MEDIUM;
	}

	String BandcampImage::Size_toString(Size size) {
		switch(size) {
			case Size::SMALL:
				return "small";
			case Size::MEDIUM:
				return "medium";
			case Size::LARGE:
				return "large";
		}
		return "medium";
	}



	BandcampSearchResults BandcampSearchResults::fromNapiObject(Napi::Object results) {
		return BandcampSearchResults{
			.prevURL = jsutils::stringFromNapiValue(results.Get("prevURL")),
			.nextURL = jsutils::stringFromNapiValue(results.Get("nextURL")),
			.items = jsutils::arrayListFromNapiArray(results.Get("items").template As<Napi::Array>(), [=](auto itemValue) -> Item {
				Napi::Object item = itemValue.template As<Napi::Object>();
				return Item{
					.type = ([&]() -> Item::Type {
						auto mediaType = item.Get("type").As<Napi::String>().Utf8Value();
						if(mediaType == "track") {
							return Item::Type::TRACK;
						} else if(mediaType == "artist") {
							return Item::Type::ARTIST;
						} else if(mediaType == "album") {
							return Item::Type::ALBUM;
						} else if(mediaType == "label") {
							return Item::Type::LABEL;
						} else if(mediaType == "fan") {
							return Item::Type::FAN;
						} else {
							return Item::Type::UNKNOWN;
						}
					})(),
					.name = item.Get("name").As<Napi::String>().Utf8Value(),
					.url = item.Get("url").As<Napi::String>().Utf8Value(),
					.imageURL = jsutils::stringFromNapiValue(item.Get("imageURL")),
					.tags = jsutils::arrayListFromNapiArray(item.Get("tags").template As<Napi::Array>(), [](auto tag) -> String {
						return tag.ToString();
					}),
					.genre = jsutils::stringFromNapiValue(item.Get("genre")),
					.releaseDate = jsutils::stringFromNapiValue(item.Get("releaseDate")),
					.artistName = jsutils::stringFromNapiValue(item.Get("artistName")),
					.artistURL = jsutils::stringFromNapiValue(item.Get("artistURL")),
					.albumName = jsutils::stringFromNapiValue(item.Get("albumName")),
					.albumURL = jsutils::stringFromNapiValue(item.Get("albumURL")),
					.location = jsutils::stringFromNapiValue(item.Get("location")),
					.numTracks = jsutils::optSizeFromNapiValue(item.Get("numTracks")),
					.numMinutes = jsutils::optSizeFromNapiValue(item.Get("numMinutes"))
				};
			})
		};
	}

	BandcampArtist BandcampArtist::fromNapiObject(Napi::Object artist) {
		return BandcampArtist{
			.url = artist.Get("url").As<Napi::String>().Utf8Value(),
			.name = artist.Get("name").As<Napi::String>().Utf8Value(),
			.location = jsutils::stringFromNapiValue(artist.Get("location")),
			.description = jsutils::stringFromNapiValue(artist.Get("description")),
			.isLabel = artist.Get("isLabel").As<Napi::Boolean>().Value(),
			.images = jsutils::arrayListFromNapiArray(artist.Get("images").As<Napi::Array>(), [](auto image) -> BandcampImage {
				return BandcampImage::fromNapiObject(image.template As<Napi::Object>());
			}),
			.shows = jsutils::arrayListFromNapiArray(artist.Get("shows").As<Napi::Array>(), [](auto show) -> BandcampShow {
				return BandcampShow::fromNapiObject(show.template As<Napi::Object>());
			}),
			.links = jsutils::arrayListFromNapiArray(artist.Get("shows").As<Napi::Array>(), [](auto link) -> BandcampLink {
				return BandcampLink::fromNapiObject(link.template As<Napi::Object>());
			}),
			.albums = jsutils::optArrayListFromNapiArray(artist.Get("albums").As<Napi::Array>(), [](auto album) -> BandcampAlbum {
				return BandcampAlbum::fromNapiObject(album.template As<Napi::Object>());
			})
		};
	}

	BandcampTrack BandcampTrack::fromNapiObject(Napi::Object track) {
		return BandcampTrack{
			.url = jsutils::nonNullStringPropFromNapiObject(track,"url"),
			.name = jsutils::nonNullStringPropFromNapiObject(track,"name"),
			.images = jsutils::arrayListFromNapiArray(track.Get("images").As<Napi::Array>(), [](auto image) -> BandcampImage {
				return BandcampImage::fromNapiObject(image.template As<Napi::Object>());
			}),
			.artistName = jsutils::nonNullStringPropFromNapiObject(track,"artistName"),
			.artistURL = jsutils::nonNullStringPropFromNapiObject(track,"artistURL"),
			.artist = ([&]() -> Optional<BandcampArtist> {
				auto artist = track.Get("artist");
				if(artist.IsEmpty() || artist.IsNull() || artist.IsUndefined()) {
					return std::nullopt;
				}
				return BandcampArtist::fromNapiObject(artist.template As<Napi::Object>());
			})(),
			.albumName = jsutils::nonNullStringPropFromNapiObject(track,"albumName"),
			.albumURL = jsutils::nonNullStringPropFromNapiObject(track,"albumURL"),
			.trackNumber = jsutils::optSizeFromNapiValue(track.Get("trackNumber")),
			.duration = jsutils::optDoubleFromNapiValue(track.Get("duration")),
			.tags = jsutils::optArrayListFromNapiArray(track.Get("tags").As<Napi::Array>(), [](auto tag) -> String {
				return tag.ToString();
			}),
			.description = jsutils::optStringFromNapiValue(track.Get("description")),
			.audioSources = jsutils::optArrayListFromNapiArray(track.Get("audioSources").As<Napi::Array>(), [](auto audioSource) -> AudioSource {
				return AudioSource::fromNapiObject(audioSource.template As<Napi::Object>());
			}),
			.playable = jsutils::optBoolFromNapiValue(track.Get("playable"))
		};
	}

	BandcampTrack::AudioSource BandcampTrack::AudioSource::fromNapiObject(Napi::Object audioSource) {
		return AudioSource{
			.type = jsutils::stringFromNapiValue(audioSource.Get("type")),
			.url = jsutils::nonNullStringPropFromNapiObject(audioSource,"url")
		};
	}

	BandcampAlbum BandcampAlbum::fromNapiObject(Napi::Object album) {
		return BandcampAlbum{
			.url = jsutils::nonNullStringPropFromNapiObject(album,"url"),
			.name = jsutils::nonNullStringPropFromNapiObject(album,"name"),
			.images = jsutils::arrayListFromNapiArray(album.Get("images").As<Napi::Array>(), [](auto image) -> BandcampImage {
				return BandcampImage::fromNapiObject(image.template As<Napi::Object>());
			}),
			.artistName = jsutils::nonNullStringPropFromNapiObject(album,"artistName"),
			.artistURL = jsutils::nonNullStringPropFromNapiObject(album,"artistURL"),
			.artist = ([&]() -> Optional<BandcampArtist> {
				auto artist = album.Get("artist");
				if(artist.IsEmpty() || artist.IsNull() || artist.IsUndefined()) {
					return std::nullopt;
				}
				return BandcampArtist::fromNapiObject(artist.template As<Napi::Object>());
			})(),
			.tags = jsutils::optArrayListFromNapiArray(album.Get("tags").As<Napi::Array>(), [](auto tag) -> String {
				return tag.ToString();
			}),
			.description = jsutils::optStringFromNapiValue(album.Get("description")),
			.numTracks = ([&]() -> Optional<size_t> {
				auto tracks = album.Get("tracks").As<Napi::Array>();
				if(!tracks.IsEmpty() && !tracks.IsNull() && !tracks.IsUndefined()) {
					return (size_t)tracks.Length();
				}
				auto numTracks = album.Get("numTracks").As<Napi::Number>();
				if(!numTracks.IsEmpty() && !numTracks.IsNull() && !numTracks.IsUndefined()) {
					return (size_t)numTracks.Int64Value();
				}
				return std::nullopt;
			})(),
			.tracks = jsutils::optArrayListFromNapiArray(album.Get("tracks").As<Napi::Array>(), [](auto track) -> BandcampTrack {
				return BandcampTrack::fromNapiObject(track.template As<Napi::Object>());
			})
		};
	}

	BandcampAlbum BandcampAlbum::fromSingle(BandcampTrack track) {
		if(track.url.empty() || track.url != track.albumURL) {
			throw BandcampError(BandcampError::Code::MEDIATYPE_MISMATCH, "Bandcamp track is not a single");
		}
		return BandcampAlbum{
			.url=track.albumURL,
			.name=track.albumName,
			.images=track.images,
			.artistName=track.artistName,
			.artistURL=track.artistURL,
			.artist=track.artist,
			.tags=track.tags,
			.description=track.description,
			.numTracks=1,
			.tracks=ArrayList<BandcampTrack>{
				track
			}
		};
	}

	BandcampShow BandcampShow::fromNapiObject(Napi::Object show) {
		return BandcampShow{
			.date = jsutils::stringFromNapiValue(show.Get("date")),
			.url = jsutils::stringFromNapiValue(show.Get("url")),
			.venueName = jsutils::stringFromNapiValue(show.Get("venueName")),
			.location = jsutils::stringFromNapiValue(show.Get("location"))
		};
	}

	BandcampLink BandcampLink::fromNapiObject(Napi::Object link) {
		return BandcampLink{
			.name = jsutils::stringFromNapiValue(link.Get("name")),
			.url = jsutils::stringFromNapiValue(link.Get("url"))
		};
	}



	Json BandcampIdentities::toJson() const {
		return Json::object{
			{ "fan", fan ? fan->toJson() : Json() }
		};
	}

	Optional<BandcampIdentities> BandcampIdentities::maybeFromJson(const Json& json) {
		if(json.is_null()) {
			return std::nullopt;
		}
		return fromJson(json);
	}

	BandcampIdentities BandcampIdentities::fromJson(const Json& json) {
		return BandcampIdentities{
			.fan = Fan::maybeFromJson(json["fan"])
		};
	}

	BandcampIdentities BandcampIdentities::fromNapiObject(Napi::Object obj) {
		return BandcampIdentities{
			.fan = Fan::maybeFromNapiObject(obj.Get("fan").As<Napi::Object>())
		};
	}


	Json BandcampIdentities::Fan::toJson() const {
		return Json::object{
			{ "id", (std::string)id },
			{ "url", (std::string)url },
			{ "name", (std::string)name },
			{ "images", images ? images->map([](const BandcampImage& image) -> Json {
				return image.toJson();
			}) : Json() }
		};
	}

	Optional<BandcampIdentities::Fan> BandcampIdentities::Fan::maybeFromJson(const Json& json) {
		if(json.is_null()) {
			return std::nullopt;
		}
		return fromJson(json);
	}

	BandcampIdentities::Fan BandcampIdentities::Fan::fromJson(const Json& json) {
		return Fan{
			.id=json["id"].string_value(),
			.url=json["url"].string_value(),
			.name=json["name"].string_value(),
			.images=jsutils::optArrayListFromJson(json["images"], [](const Json& json) -> BandcampImage {
				return BandcampImage::fromJson(json);
			})
		};
	}

	Optional<BandcampIdentities::Fan> BandcampIdentities::Fan::maybeFromNapiObject(Napi::Object obj) {
		if(obj.IsEmpty() || obj.IsNull() || obj.IsUndefined()) {
			return std::nullopt;
		}
		return Fan::fromNapiObject(obj);
	}

	BandcampIdentities::Fan BandcampIdentities::Fan::fromNapiObject(Napi::Object obj) {
		return Fan{
			.id=jsutils::nonNullStringPropFromNapiObject(obj,"id"),
			.url=jsutils::nonNullStringPropFromNapiObject(obj,"url"),
			.name=jsutils::nonNullStringPropFromNapiObject(obj,"name"),
			.images=jsutils::optArrayListFromNapiValue(obj.Get("images"), [](Napi::Value value) -> BandcampImage {
				return BandcampImage::fromNapiObject(value.template As<Napi::Object>());
			})
		};
	}



	BandcampFan BandcampFan::fromNapiObject(Napi::Object obj) {
		return BandcampFan{
			.id=jsutils::nonNullStringPropFromNapiObject(obj,"id"),
			.url=jsutils::nonNullStringPropFromNapiObject(obj,"url"),
			.name=jsutils::nonNullStringPropFromNapiObject(obj,"name"),
			.description=jsutils::stringFromNapiValue(obj.Get("description")),
			.images=jsutils::optArrayListFromNapiValue(obj.Get("images"), [](Napi::Value value) -> BandcampImage {
				return BandcampImage::fromNapiObject(value.template As<Napi::Object>());
			}),
			.collection=Section<CollectionItemNode>::maybeFromNapiObject(obj.Get("collection").As<Napi::Object>()),
			.wishlist=Section<CollectionItemNode>::maybeFromNapiObject(obj.Get("wishlist").As<Napi::Object>()),
			.hiddenCollection=Section<CollectionItemNode>::maybeFromNapiObject(obj.Get("hiddenCollection").As<Napi::Object>()),
			.followingArtists=Section<FollowItemNode<CollectionArtist>>::maybeFromNapiObject(obj.Get("followingArtists").As<Napi::Object>()),
			.followingFans=Section<FollowItemNode<CollectionFan>>::maybeFromNapiObject(obj.Get("followingFans").As<Napi::Object>()),
			.followers=Section<FollowItemNode<CollectionFan>>::maybeFromNapiObject(obj.Get("followers").As<Napi::Object>())
		};
	}

	BandcampFan::CollectionItemNode BandcampFan::CollectionItemNode::fromNapiObject(Napi::Object obj) {
		return CollectionItemNode{
			.token=jsutils::nonNullStringPropFromNapiObject(obj,"token"),
			.dateAdded=jsutils::nonNullStringPropFromNapiObject(obj,"dateAdded"),
			.item=([&]() -> ItemVariant {
				auto item = obj.Get("item").As<Napi::Object>();
				auto itemType = jsutils::nonNullStringPropFromNapiObject(item,"type");
				if(itemType == "track") {
					return CollectionTrack::fromNapiObject(item);
				} else if(itemType == "album") {
					return CollectionAlbum::fromNapiObject(item);
				} else {
					throw std::invalid_argument("invalid item type "+itemType);
				}
			})()
		};
	}

	Optional<BandcampFan::CollectionTrack> BandcampFan::CollectionItemNode::trackItem() const {
		if(auto track = std::get_if<CollectionTrack>(&item)) {
			return *track;
		}
		return std::nullopt;
	}

	Optional<BandcampFan::CollectionAlbum> BandcampFan::CollectionItemNode::albumItem() const {
		if(auto album = std::get_if<CollectionAlbum>(&item)) {
			return *album;
		}
		return std::nullopt;
	}

	BandcampFan::CollectionTrack BandcampFan::CollectionTrack::fromNapiObject(Napi::Object obj) {
		return CollectionTrack{
			.url=obj.Get("url").As<Napi::String>().Utf8Value(),
			.images=jsutils::optArrayListFromNapiValue(obj.Get("images"), [](Napi::Value value) -> BandcampImage {
				return BandcampImage::fromNapiObject(value.template As<Napi::Object>());
			}),
			.name=jsutils::nonNullStringPropFromNapiObject(obj,"name"),
			.artistName=jsutils::nonNullStringPropFromNapiObject(obj,"artistName"),
			.artistURL=jsutils::nonNullStringPropFromNapiObject(obj,"artistURL"),
			.duration=jsutils::optDoubleFromNapiValue(obj.Get("duration")),
			.trackNumber=jsutils::sizeFromNapiValue(obj.Get("trackNumber")),
			.albumURL=jsutils::nonNullStringPropFromNapiObject(obj,"albumURL"),
			.albumName=jsutils::optStringFromNapiValue(obj.Get("albumName")),
			.albumSlug=jsutils::optStringFromNapiValue(obj.Get("albumSlug")),
		};
	}

	BandcampFan::CollectionAlbum BandcampFan::CollectionAlbum::fromNapiObject(Napi::Object obj) {
		return CollectionAlbum{
			.url=jsutils::nonNullStringPropFromNapiObject(obj,"url"),
			.name=jsutils::nonNullStringPropFromNapiObject(obj,"name"),
			.images=jsutils::optArrayListFromNapiValue(obj.Get("images"), [](Napi::Value value) -> BandcampImage {
				return BandcampImage::fromNapiObject(value.template As<Napi::Object>());
			}),
			.artistName=jsutils::nonNullStringPropFromNapiObject(obj,"artistName"),
			.artistURL=jsutils::nonNullStringPropFromNapiObject(obj,"artistURL")
		};
	}

	BandcampFan::CollectionArtist BandcampFan::CollectionArtist::fromNapiObject(Napi::Object obj) {
		return CollectionArtist{
			.url=jsutils::nonNullStringPropFromNapiObject(obj,"url"),
			.name=jsutils::nonNullStringPropFromNapiObject(obj,"name"),
			.location=jsutils::optStringFromNapiValue(obj.Get("location")),
			.images=jsutils::optArrayListFromNapiValue(obj.Get("images"), [](Napi::Value value) -> BandcampImage {
				return BandcampImage::fromNapiObject(value.template As<Napi::Object>());
			})
		};
	}

	BandcampFan::CollectionFan BandcampFan::CollectionFan::fromNapiObject(Napi::Object obj) {
		return CollectionFan{
			.id=jsutils::nonNullStringPropFromNapiObject(obj,"id"),
			.url=jsutils::nonNullStringPropFromNapiObject(obj,"url"),
			.name=jsutils::nonNullStringPropFromNapiObject(obj,"name"),
			.location=jsutils::optStringFromNapiValue(obj.Get("location")),
			.images=jsutils::optArrayListFromNapiValue(obj.Get("images"), [](Napi::Value value) -> BandcampImage {
				return BandcampImage::fromNapiObject(value.template As<Napi::Object>());
			})
		};
	}
}

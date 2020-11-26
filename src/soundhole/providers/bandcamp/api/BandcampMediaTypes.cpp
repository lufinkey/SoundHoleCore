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
			.items = jsutils::arrayListFromNapiArray<Item>(results.Get("items").template As<Napi::Array>(), [=](auto itemValue) -> Item {
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
					.tags = jsutils::arrayListFromNapiArray<String>(item.Get("tags").template As<Napi::Array>(), [](auto tag) -> String {
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
			.location = artist.Get("location").As<Napi::String>().Utf8Value(),
			.description = artist.Get("description").As<Napi::String>().Utf8Value(),
			.isLabel = artist.Get("isLabel").As<Napi::Boolean>().Value(),
			.images = jsutils::arrayListFromNapiArray<BandcampImage>(artist.Get("images").As<Napi::Array>(), [](auto image) -> BandcampImage {
				return BandcampImage::fromNapiObject(image.template As<Napi::Object>());
			}),
			.shows = jsutils::arrayListFromNapiArray<BandcampShow>(artist.Get("shows").As<Napi::Array>(), [](auto show) -> BandcampShow {
				return BandcampShow::fromNapiObject(show.template As<Napi::Object>());
			}),
			.links = jsutils::arrayListFromNapiArray<BandcampLink>(artist.Get("shows").As<Napi::Array>(), [](auto link) -> BandcampLink {
				return BandcampLink::fromNapiObject(link.template As<Napi::Object>());
			}),
			.albums = jsutils::optArrayListFromNapiArray<BandcampAlbum>(artist.Get("albums").As<Napi::Array>(), [](auto album) -> BandcampAlbum {
				return BandcampAlbum::fromNapiObject(album.template As<Napi::Object>());
			})
		};
	}

	BandcampTrack BandcampTrack::fromNapiObject(Napi::Object track) {
		return BandcampTrack{
			.url = jsutils::stringFromNapiValue(track.Get("url")),
			.name = jsutils::stringFromNapiValue(track.Get("name")),
			.images = jsutils::arrayListFromNapiArray<BandcampImage>(track.Get("images").As<Napi::Array>(), [](auto image) -> BandcampImage {
				return BandcampImage::fromNapiObject(image.template As<Napi::Object>());
			}),
			.artistName = jsutils::stringFromNapiValue(track.Get("artistName")),
			.artistURL = jsutils::stringFromNapiValue(track.Get("artistURL")),
			.artist = ([&]() -> Optional<BandcampArtist> {
				auto artist = track.Get("artist");
				if(artist.IsEmpty() || artist.IsNull() || artist.IsUndefined()) {
					return std::nullopt;
				}
				return BandcampArtist::fromNapiObject(artist.template As<Napi::Object>());
			})(),
			.albumName = jsutils::stringFromNapiValue(track.Get("albumName")),
			.albumURL = jsutils::stringFromNapiValue(track.Get("albumURL")),
			.duration = jsutils::optDoubleFromNapiValue(track.Get("duration")),
			.tags = jsutils::optArrayListFromNapiArray<String>(track.Get("tags").As<Napi::Array>(), [](auto tag) -> String {
				return tag.ToString();
			}),
			.description = jsutils::optStringFromNapiValue(track.Get("description")),
			.audioSources = jsutils::optArrayListFromNapiArray<AudioSource>(track.Get("audioSources").As<Napi::Array>(), [](auto audioSource) -> AudioSource {
				return AudioSource::fromNapiObject(audioSource.template As<Napi::Object>());
			}),
			.playable = jsutils::optBoolFromNapiValue(track.Get("playable"))
		};
	}

	BandcampTrack::AudioSource BandcampTrack::AudioSource::fromNapiObject(Napi::Object audioSource) {
		return AudioSource{
			.type = jsutils::stringFromNapiValue(audioSource.Get("type")),
			.url = audioSource.Get("url").As<Napi::String>()
		};
	}

	BandcampAlbum BandcampAlbum::fromNapiObject(Napi::Object album) {
		return BandcampAlbum{
			.url = album.Get("url").As<Napi::String>().Utf8Value(),
			.name = album.Get("name").As<Napi::String>().Utf8Value(),
			.images = jsutils::arrayListFromNapiArray<BandcampImage>(album.Get("images").As<Napi::Array>(), [](auto image) -> BandcampImage {
				return BandcampImage::fromNapiObject(image.template As<Napi::Object>());
			}),
			.artistName = jsutils::stringFromNapiValue(album.Get("artistName")),
			.artistURL = jsutils::stringFromNapiValue(album.Get("artistURL")),
			.artist = ([&]() -> Optional<BandcampArtist> {
				auto artist = album.Get("artist");
				if(artist.IsEmpty() || artist.IsNull() || artist.IsUndefined()) {
					return std::nullopt;
				}
				return BandcampArtist::fromNapiObject(artist.template As<Napi::Object>());
			})(),
			.tags = jsutils::optArrayListFromNapiArray<String>(album.Get("tags").As<Napi::Array>(), [](auto tag) -> String {
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
			.tracks = jsutils::optArrayListFromNapiArray<BandcampTrack>(album.Get("tracks").As<Napi::Array>(), [](auto track) -> BandcampTrack {
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
			{ "username", (std::string)username },
			{ "name", (std::string)name },
			{ "images", images ? images->map<Json>([](const BandcampImage& image) {
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
			.username=json["username"].string_value(),
			.name=json["name"].string_value(),
			.images=jsutils::optArrayListFromJson<BandcampImage>(json["images"], [](const Json& json) {
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
			.id=obj.Get("id").As<Napi::String>().Utf8Value(),
			.url=obj.Get("url").As<Napi::String>().Utf8Value(),
			.username=obj.Get("username").As<Napi::String>().Utf8Value(),
			.name=obj.Get("name").As<Napi::String>().Utf8Value(),
			.images=jsutils::optArrayListFromNapiValue<BandcampImage>(obj.Get("images"), [](Napi::Value value) {
				return BandcampImage::fromNapiObject(value.template As<Napi::Object>());
			})
		};
	}



	BandcampFan BandcampFan::fromNapiObject(Napi::Object obj) {
		return BandcampFan{
			.id=obj.Get("id").As<Napi::String>().Utf8Value(),
			.url=obj.Get("url").As<Napi::String>().Utf8Value(),
			.username=obj.Get("username").As<Napi::String>().Utf8Value(),
			.name=obj.Get("name").As<Napi::String>().Utf8Value(),
			.description=jsutils::stringFromNapiValue(obj.Get("description")),
			.images=jsutils::optArrayListFromNapiValue<BandcampImage>(obj.Get("images"), [](Napi::Value value) {
				return BandcampImage::fromNapiObject(value.template As<Napi::Object>());
			}),
			.collection=Section<CollectionItemNode>::maybeFromNapiObject(obj.Get("collection").As<Napi::Object>()),
			.wishlist=Section<CollectionItemNode>::maybeFromNapiObject(obj.Get("wishlist").As<Napi::Object>()),
			.followingArtists=Section<FollowItemNode<CollectionArtist>>::maybeFromNapiObject(obj.Get("followingArtists").As<Napi::Object>()),
			.followingFans=Section<FollowItemNode<CollectionFan>>::maybeFromNapiObject(obj.Get("followingFans").As<Napi::Object>()),
			.followers=Section<FollowItemNode<CollectionFan>>::maybeFromNapiObject(obj.Get("followers").As<Napi::Object>())
		};
	}

	BandcampFan::CollectionItemNode BandcampFan::CollectionItemNode::fromNapiObject(Napi::Object obj) {
		return CollectionItemNode{
			.token=obj.Get("token").As<Napi::String>().Utf8Value(),
			.dateAdded=obj.Get("dateAdded").As<Napi::String>().Utf8Value(),
			.item=([&]() -> ItemVariant {
				auto item = obj.Get("item").As<Napi::Object>();
				auto itemType = item.Get("type").As<Napi::String>().Utf8Value();
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
			.images=jsutils::optArrayListFromNapiValue<BandcampImage>(obj.Get("images"), [](Napi::Value value) {
				return BandcampImage::fromNapiObject(value.template As<Napi::Object>());
			}),
			.name=obj.Get("name").As<Napi::String>().Utf8Value(),
			.artistName=obj.Get("artistName").As<Napi::String>().Utf8Value(),
			.artistURL=obj.Get("artistURL").As<Napi::String>().Utf8Value(),
			.duration=jsutils::optDoubleFromNapiValue(obj.Get("duration")),
			.trackNumber=jsutils::sizeFromNapiValue(obj.Get("trackNumber")),
			.albumURL=obj.Get("albumURL").As<Napi::String>().Utf8Value(),
			.albumName=obj.Get("albumName").As<Napi::String>().Utf8Value(),
			.albumSlug=obj.Get("albumSlug").As<Napi::String>().Utf8Value(),
		};
	}

	BandcampFan::CollectionAlbum BandcampFan::CollectionAlbum::fromNapiObject(Napi::Object obj) {
		return CollectionAlbum{
			.url=obj.Get("url").As<Napi::String>().Utf8Value(),
			.name=obj.Get("name").As<Napi::String>().Utf8Value(),
			.images=jsutils::optArrayListFromNapiValue<BandcampImage>(obj.Get("images"), [](Napi::Value value) {
				return BandcampImage::fromNapiObject(value.template As<Napi::Object>());
			}),
			.artistName=obj.Get("artistName").As<Napi::String>().Utf8Value(),
			.artistURL=obj.Get("artistURL").As<Napi::String>().Utf8Value()
		};
	}

	BandcampFan::CollectionArtist BandcampFan::CollectionArtist::fromNapiObject(Napi::Object obj) {
		return CollectionArtist{
			.url=obj.Get("url").As<Napi::String>().Utf8Value(),
			.name=obj.Get("name").As<Napi::String>().Utf8Value(),
			.location=jsutils::optStringFromNapiValue(obj.Get("location")),
			.images=jsutils::optArrayListFromNapiValue<BandcampImage>(obj.Get("images"), [](Napi::Value value) {
				return BandcampImage::fromNapiObject(value.template As<Napi::Object>());
			})
		};
	}

	BandcampFan::CollectionFan BandcampFan::CollectionFan::fromNapiObject(Napi::Object obj) {
		return CollectionFan{
			.id=obj.Get("id").As<Napi::String>().Utf8Value(),
			.url=obj.Get("url").As<Napi::String>().Utf8Value(),
			.name=obj.Get("name").As<Napi::String>().Utf8Value(),
			.location=jsutils::optStringFromNapiValue(obj.Get("location")),
			.images=jsutils::optArrayListFromNapiValue<BandcampImage>(obj.Get("images"), [](Napi::Value value) {
				return BandcampImage::fromNapiObject(value.template As<Napi::Object>());
			})
		};
	}
}

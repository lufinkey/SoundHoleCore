//
//  BandcampMediaTypes.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/21/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include <napi.h>
#include "BandcampMediaTypes.hpp"
#include <soundhole/utils/js/JSWrapClass.hpp>

namespace sh {
	BandcampImage BandcampImage::fromNapiObject(Napi::Object image) {
		return BandcampImage{
			.url = JSWrapClass::stringFromNapiValue(image.Get("url")),
			.size = ([&]() -> Size {
				auto size = JSWrapClass::stringFromNapiValue(image.Get("size"));
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

	BandcampSearchResults BandcampSearchResults::fromNapiObject(Napi::Object results) {
		return BandcampSearchResults{
			.prevURL = JSWrapClass::stringFromNapiValue(results.Get("prevURL")),
			.nextURL = JSWrapClass::stringFromNapiValue(results.Get("nextURL")),
			.items = JSWrapClass::arrayListFromNapiArray<Item>(results.Get("items").template As<Napi::Array>(), [=](auto itemValue) -> Item {
				auto item = itemValue.template As<Napi::Object>();
				return Item{
					.type = ([&]() -> Item::Type {
						auto mediaType = JSWrapClass::stringFromNapiValue(item.Get("type"));
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
					.name = JSWrapClass::stringFromNapiValue(item.Get("name")),
					.url = JSWrapClass::stringFromNapiValue(item.Get("url")),
					.imageURL = JSWrapClass::stringFromNapiValue(item.Get("imageURL")),
					.tags = JSWrapClass::arrayListFromNapiArray<String>(item.Get("tags").template As<Napi::Array>(), [](auto tag) -> String {
						return tag.ToString();
					}),
					.genre = JSWrapClass::stringFromNapiValue(item.Get("genre")),
					.releaseDate = JSWrapClass::stringFromNapiValue(item.Get("releaseDate")),
					.artistName = JSWrapClass::stringFromNapiValue(item.Get("artistName")),
					.artistURL = JSWrapClass::stringFromNapiValue(item.Get("artistURL")),
					.albumName = JSWrapClass::stringFromNapiValue(item.Get("albumName")),
					.albumURL = JSWrapClass::stringFromNapiValue(item.Get("albumURL")),
					.location = JSWrapClass::stringFromNapiValue(item.Get("location")),
					.numTracks = ([&]() -> Optional<size_t> {
						auto numTracks = item.Get("numTracks");
						if(numTracks.IsEmpty() || numTracks.IsNull() || numTracks.IsUndefined()) {
							return std::nullopt;
						}
						return Optional<size_t>(numTracks.template As<Napi::Number>().Int64Value());
					})(),
					.numMinutes = ([&]() -> Optional<size_t> {
						auto numMinutes = item.Get("numMinutes");
						if(numMinutes.IsEmpty() || numMinutes.IsNull() || numMinutes.IsUndefined()) {
							return std::nullopt;
						}
						return Optional<size_t>(numMinutes.template As<Napi::Number>().Int64Value());
					})()
				};
			})
		};
	}

	BandcampArtist BandcampArtist::fromNapiObject(Napi::Object artist) {
		auto type = JSWrapClass::stringFromNapiValue(artist.Get("type"));
		return BandcampArtist{
			.url = JSWrapClass::stringFromNapiValue(artist.Get("url")),
			.name = JSWrapClass::stringFromNapiValue(artist.Get("name")),
			.location = JSWrapClass::stringFromNapiValue(artist.Get("location")),
			.description = JSWrapClass::stringFromNapiValue(artist.Get("description")),
			.images = JSWrapClass::arrayListFromNapiArray<BandcampImage>(artist.Get("images").As<Napi::Array>(), [](auto image) -> BandcampImage {
				return BandcampImage::fromNapiObject(image.template As<Napi::Object>());
			}),
			.shows = JSWrapClass::arrayListFromNapiArray<BandcampShow>(artist.Get("shows").As<Napi::Array>(), [](auto show) -> BandcampShow {
				return BandcampShow::fromNapiObject(show.template As<Napi::Object>());
			}),
			.links = JSWrapClass::arrayListFromNapiArray<BandcampLink>(artist.Get("shows").As<Napi::Array>(), [](auto link) -> BandcampLink {
				return BandcampLink::fromNapiObject(link.template As<Napi::Object>());
			}),
			.albums = JSWrapClass::optArrayListFromNapiArray<BandcampAlbum>(artist.Get("albums").As<Napi::Array>(), [](auto album) -> BandcampAlbum {
				return BandcampAlbum::fromNapiObject(album.template As<Napi::Object>());
			}),
			.isLabel = (type == "label")
		};
	}

	BandcampTrack BandcampTrack::fromNapiObject(Napi::Object track) {
		return BandcampTrack{
			.url = JSWrapClass::stringFromNapiValue(track.Get("url")),
			.name = JSWrapClass::stringFromNapiValue(track.Get("name")),
			.images = JSWrapClass::arrayListFromNapiArray<BandcampImage>(track.Get("images").As<Napi::Array>(), [](auto image) -> BandcampImage {
				return BandcampImage::fromNapiObject(image.template As<Napi::Object>());
			}),
			.artistName = JSWrapClass::stringFromNapiValue(track.Get("artistName")),
			.artistURL = JSWrapClass::stringFromNapiValue(track.Get("artistURL")),
			.artist = ([&]() -> Optional<BandcampArtist> {
				auto artist = track.Get("artist");
				if(artist.IsEmpty() || artist.IsNull() || artist.IsUndefined()) {
					return std::nullopt;
				}
				return BandcampArtist::fromNapiObject(artist.template As<Napi::Object>());
			})(),
			.albumName = JSWrapClass::stringFromNapiValue(track.Get("albumName")),
			.albumURL = JSWrapClass::stringFromNapiValue(track.Get("albumURL")),
			.audioURL = JSWrapClass::optStringFromNapiValue(track.Get("audioURL")),
			.duration = ([&]() -> Optional<double> {
				auto duration = track.Get("duration");
				if(duration.IsNull() || duration.IsUndefined()) {
					return std::nullopt;
				}
				return duration.template As<Napi::Number>().DoubleValue();
			})(),
			.tags = JSWrapClass::optArrayListFromNapiArray<String>(track.Get("tags").As<Napi::Array>(), [](auto tag) -> String {
				return tag.ToString();
			}),
			.description = JSWrapClass::optStringFromNapiValue(track.Get("description"))
		};
	}

	BandcampAlbum BandcampAlbum::fromNapiObject(Napi::Object album) {
		return BandcampAlbum{
			.url = JSWrapClass::stringFromNapiValue(album.Get("url")),
			.name = JSWrapClass::stringFromNapiValue(album.Get("name")),
			.images = JSWrapClass::arrayListFromNapiArray<BandcampImage>(album.Get("images").As<Napi::Array>(), [](auto image) -> BandcampImage {
				return BandcampImage::fromNapiObject(image.template As<Napi::Object>());
			}),
			.artistName = JSWrapClass::stringFromNapiValue(album.Get("artistName")),
			.artistURL = JSWrapClass::stringFromNapiValue(album.Get("artistURL")),
			.artist = ([&]() -> Optional<BandcampArtist> {
				auto artist = album.Get("artist");
				if(artist.IsEmpty() || artist.IsNull() || artist.IsUndefined()) {
					return std::nullopt;
				}
				return BandcampArtist::fromNapiObject(artist.template As<Napi::Object>());
			})(),
			.tracks = JSWrapClass::optArrayListFromNapiArray<BandcampTrack>(album.Get("tracks").As<Napi::Array>(), [](auto track) -> BandcampTrack {
				return BandcampTrack::fromNapiObject(track.template As<Napi::Object>());
			}),
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
			.tags = JSWrapClass::optArrayListFromNapiArray<String>(album.Get("tags").As<Napi::Array>(), [](auto tag) -> String {
				return tag.ToString();
			}),
			.description = JSWrapClass::optStringFromNapiValue(album.Get("description"))
		};
	}

	BandcampShow BandcampShow::fromNapiObject(Napi::Object show) {
		return BandcampShow{
			.date = JSWrapClass::stringFromNapiValue(show.Get("date")),
			.url = JSWrapClass::stringFromNapiValue(show.Get("url")),
			.venueName = JSWrapClass::stringFromNapiValue(show.Get("venueName")),
			.location = JSWrapClass::stringFromNapiValue(show.Get("location"))
		};
	}

	BandcampLink BandcampLink::fromNapiObject(Napi::Object link) {
		return BandcampLink{
			.name = JSWrapClass::stringFromNapiValue(link.Get("name")),
			.url = JSWrapClass::stringFromNapiValue(link.Get("url"))
		};
	}
}

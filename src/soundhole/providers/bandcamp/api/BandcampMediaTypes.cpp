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
	BandcampImage BandcampImage::fromNapiObject(Napi::Object image) {
		return BandcampImage{
			.url = jsutils::stringFromNapiValue(image.Get("url")),
			.size = ([&]() -> Size {
				auto size = jsutils::stringFromNapiValue(image.Get("size"));
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
			.prevURL = jsutils::stringFromNapiValue(results.Get("prevURL")),
			.nextURL = jsutils::stringFromNapiValue(results.Get("nextURL")),
			.items = jsutils::arrayListFromNapiArray<Item>(results.Get("items").template As<Napi::Array>(), [=](auto itemValue) -> Item {
				auto item = itemValue.template As<Napi::Object>();
				return Item{
					.type = ([&]() -> Item::Type {
						auto mediaType = jsutils::stringFromNapiValue(item.Get("type"));
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
					.name = jsutils::stringFromNapiValue(item.Get("name")),
					.url = jsutils::stringFromNapiValue(item.Get("url")),
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
		auto type = jsutils::stringFromNapiValue(artist.Get("type"));
		return BandcampArtist{
			.url = jsutils::stringFromNapiValue(artist.Get("url")),
			.name = jsutils::stringFromNapiValue(artist.Get("name")),
			.location = jsutils::stringFromNapiValue(artist.Get("location")),
			.description = jsutils::stringFromNapiValue(artist.Get("description")),
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
			}),
			.isLabel = (type == "label")
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
			.url = jsutils::stringFromNapiValue(audioSource.Get("url"))
		};
	}

	BandcampAlbum BandcampAlbum::fromNapiObject(Napi::Object album) {
		return BandcampAlbum{
			.url = jsutils::stringFromNapiValue(album.Get("url")),
			.name = jsutils::stringFromNapiValue(album.Get("name")),
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
}

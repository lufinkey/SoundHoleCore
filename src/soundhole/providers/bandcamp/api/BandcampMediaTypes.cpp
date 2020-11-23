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



	BandcampIdentities BandcampIdentities::fromNapiObject(Napi::Object obj) {
		return BandcampIdentities{
			.fan = Fan::maybeFromNapiObject(obj.Get("fan").As<Napi::Object>())
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

	BandcampFan::CollectionTrack BandcampFan::CollectionTrack::fromNapiObject(Napi::Object obj) {
		return CollectionTrack{
			.type=obj.Get("type").As<Napi::String>().Utf8Value(),
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
			.type=obj.Get("type").As<Napi::String>().Utf8Value(),
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
			.type=obj.Get("type").As<Napi::String>().Utf8Value(),
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
			.type=obj.Get("type").As<Napi::String>().Utf8Value(),
			.url=obj.Get("url").As<Napi::String>().Utf8Value(),
			.name=obj.Get("name").As<Napi::String>().Utf8Value(),
			.location=jsutils::optStringFromNapiValue(obj.Get("location")),
			.images=jsutils::optArrayListFromNapiValue<BandcampImage>(obj.Get("images"), [](Napi::Value value) {
				return BandcampImage::fromNapiObject(value.template As<Napi::Object>());
			})
		};
	}
}

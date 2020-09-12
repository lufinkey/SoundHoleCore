//
//  Album.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "Album.hpp"
#include "MediaProvider.hpp"

namespace sh {
	$<AlbumItem> AlbumItem::new$($<SpecialTrackCollection<AlbumItem>> album, const Data& data) {
		return fgl::new$<AlbumItem>(album, data);
	}

	AlbumItem::AlbumItem($<SpecialTrackCollection<AlbumItem>> album, const Data& data)
	: SpecialTrackCollectionItem<Album>(album, data) {
		//
	}

	bool AlbumItem::matchesItem(const TrackCollectionItem* item) const {
		auto albumItem = dynamic_cast<const AlbumItem*>(item);
		if(albumItem == nullptr) {
			return false;
		}
		if(_track->trackNumber() == albumItem->_track->trackNumber() && _track->uri() == albumItem->_track->uri()) {
			return true;
		}
		return false;
	}

	AlbumItem::AlbumItem($<SpecialTrackCollection<AlbumItem>> album, const Json& json, MediaProviderStash* stash)
	: SpecialTrackCollectionItem<Album>(album, json, stash) {
		//
	}

	$<AlbumItem> AlbumItem::fromJson($<SpecialTrackCollection<AlbumItem>> album, const Json& json, MediaProviderStash* stash) {
		return fgl::new$<AlbumItem>(album, json, stash);
	}



	$<Album> Album::new$(MediaProvider* provider, const Data& data) {
		return fgl::new$<Album>(provider, data);
	}

	Album::Album(MediaProvider* provider, const Data& data)
	: SpecialTrackCollection<AlbumItem>(provider, data),
	_artists(data.artists) {
		//
	}
	
	const ArrayList<$<Artist>>& Album::artists() {
		return _artists;
	}

	const ArrayList<$<const Artist>>& Album::artists() const {
		return *((const ArrayList<$<const Artist>>*)(&_artists));
	}

	Promise<void> Album::fetchData() {
		auto self = std::static_pointer_cast<Album>(shared_from_this());
		return provider->getAlbumData(_uri).then([=](Data data) {
			self->applyData(data);
		});
	}

	void Album::applyData(const Data& data) {
		SpecialTrackCollection<AlbumItem>::applyData(data);
		_artists = data.artists.map<$<Artist>>([&](auto& artist) {
			auto cmpArtist = _artists.firstWhere([&](auto& cmpArtist) { return artist->uri() == cmpArtist->uri(); }, nullptr);
			if(cmpArtist) {
				return cmpArtist;
			}
			return artist;
		});
	}

	Album::Data Album::toData(const DataOptions& options) const {
		return Album::Data{
			SpecialTrackCollection<AlbumItem>::toData(options),
			.artists=_artists
		};
	}

	$<Album> Album::fromJson(const Json& json, MediaProviderStash* stash) {
		auto album = fgl::new$<Album>(json, stash);
		album->lazyLoadContentIfNeeded();
		return album;
	}

	Album::Album(const Json& json, MediaProviderStash* stash)
	: SpecialTrackCollection<AlbumItem>(json, stash) {
		auto artists = json["artists"];
		_artists.reserve(artists.array_items().size());
		for(auto& artist : artists.array_items()) {
			_artists.pushBack(Artist::fromJson(artist, stash));
		}
	}

	Json Album::toJson(const ToJsonOptions& options) const {
		auto json = SpecialTrackCollection<AlbumItem>::toJson(options).object_items();
		json.merge(Json::object{
			{ "artists", (Json::array)_artists.map<Json>([&](auto& artist) {
				return artist->toJson();
			}) }
		});
		return json;
	}

	Album::MutatorDelegate* Album::createMutatorDelegate() {
		return provider->createAlbumMutatorDelegate(std::static_pointer_cast<Album>(shared_from_this()));
	}
}

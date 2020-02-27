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
	$<AlbumItem> AlbumItem::new$($<SpecialTrackCollection<AlbumItem>> album, Data data) {
		$<TrackCollectionItem> ptr;
		new AlbumItem(ptr, album, data);
		return std::static_pointer_cast<AlbumItem>(ptr);
	}

	AlbumItem::AlbumItem($<TrackCollectionItem>& ptr, $<SpecialTrackCollection<AlbumItem>> album, Data data)
	: SpecialTrackCollectionItem<Album>(ptr, album, data) {
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

	AlbumItem::AlbumItem($<TrackCollectionItem>& ptr, $<SpecialTrackCollection<AlbumItem>> album, Json json, MediaProviderStash* stash)
	: SpecialTrackCollectionItem<Album>(ptr, album, json, stash) {
		//
	}

	$<AlbumItem> AlbumItem::fromJson($<SpecialTrackCollection<AlbumItem>> album, Json json, MediaProviderStash* stash) {
		$<TrackCollectionItem> ptr;
		new AlbumItem(ptr, album, json, stash);
		return std::static_pointer_cast<AlbumItem>(ptr);
	}



	$<Album> Album::new$(MediaProvider* provider, Data data) {
		$<MediaItem> ptr;
		new Album(ptr, provider, data);
		return std::static_pointer_cast<Album>(ptr);
	}

	Album::Album($<MediaItem>& ptr, MediaProvider* provider, Data data)
	: SpecialTrackCollection<AlbumItem>(ptr, provider, data),
	_artists(data.artists) {
		//
	}
	
	const ArrayList<$<Artist>>& Album::artists() {
		return _artists;
	}

	const ArrayList<$<const Artist>>& Album::artists() const {
		return *((const ArrayList<$<const Artist>>*)(&_artists));
	}

	bool Album::needsData() const {
		return tracksAreEmpty();
	}

	Promise<void> Album::fetchMissingData() {
		return provider->getAlbumData(_uri).then([=](Data data) {
			if(tracksAreEmpty()) {
				_items = constructItems(data.tracks);
			}
		});
	}

	Album::Data Album::toData(DataOptions options) const {
		return Album::Data{
			SpecialTrackCollection<AlbumItem>::toData(options),
			.artists=_artists
		};
	}

	$<Album> Album::fromJson(Json json, MediaProviderStash* stash) {
		$<MediaItem> ptr;
		new Album(ptr, json, stash);
		return std::static_pointer_cast<Album>(ptr);
	}

	Album::Album($<MediaItem>& ptr, Json json, MediaProviderStash* stash)
	: SpecialTrackCollection<AlbumItem>(ptr, json, stash) {
		auto artists = json["artists"];
		_artists.reserve(artists.array_items().size());
		for(auto& artist : artists.array_items()) {
			_artists.pushBack(Artist::fromJson(artist, stash));
		}
	}

	Json Album::toJson(ToJsonOptions options) const {
		auto json = SpecialTrackCollection<AlbumItem>::toJson(options).object_items();
		json.merge(Json::object{
			{ "artists", _artists.map<Json>([&](auto& artist) {
				return artist->toJson();
			}).storage }
		});
		return json;
	}

	Album::MutatorDelegate* Album::createMutatorDelegate() {
		return provider->createAlbumMutatorDelegate(std::static_pointer_cast<Album>(self()));
	}
}

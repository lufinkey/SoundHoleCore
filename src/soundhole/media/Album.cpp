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

	#pragma mark AlbumItem

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



	#pragma mark Album::Data

	Album::Data Album::Data::fromJson(const Json& json, MediaProviderStash* stash) {
		auto collectionData = SpecialTrackCollection<AlbumItem>::Data::fromJson(json, stash);
		auto artistsJson = json["artists"];
		ArrayList<$<Artist>> artists;
		artists.reserve(artistsJson.array_items().size());
		for(auto& artistJson : artistsJson.array_items()) {
			auto providerName = artistJson["provider"];
			if(!providerName.is_string()) {
				throw std::invalid_argument("artist provider must be a string");
			}
			auto provider = stash->getMediaProvider(providerName.string_value());
			if(provider == nullptr) {
				throw std::invalid_argument("invalid provider name for artist: "+providerName.string_value());
			}
			artists.pushBack(provider->artist(Artist::Data::fromJson(artistJson, stash)));
		}
		return Album::Data{
			collectionData,
			.artists=artists
		};
	}



	#pragma mark Album

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
		_artists = data.artists.map([&](auto& artist) -> $<Artist> {
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

	Json Album::toJson(const ToJsonOptions& options) const {
		auto json = SpecialTrackCollection<AlbumItem>::toJson(options).object_items();
		json.merge(Json::object{
			{ "artists", _artists.map([&](auto& artist) -> Json {
				return artist->toJson();
			}) }
		});
		return json;
	}

	Album::MutatorDelegate* Album::createMutatorDelegate() {
		return provider->createAlbumMutatorDelegate(std::static_pointer_cast<Album>(shared_from_this()));
	}
}

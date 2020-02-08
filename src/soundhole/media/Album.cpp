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
	$<AlbumItem> AlbumItem::new$($<Album> album, Data data) {
		return fgl::new$<AlbumItem>(album, data);
	}

	$<AlbumItem> AlbumItem::new$($<SpecialTrackCollection<AlbumItem>> album, Data data) {
		return fgl::new$<AlbumItem>(std::static_pointer_cast<Album>(album), data);
	}

	AlbumItem::AlbumItem($<Album> album, Data data)
	: SpecialTrackCollectionItem<Album>(std::static_pointer_cast<TrackCollection>(album), data) {
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



	$<Album> Album::new$(MediaProvider* provider, Data data) {
		return fgl::new$<Album>(provider, data);
	}

	Album::Album(MediaProvider* provider, Data data)
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

	Album::MutatorDelegate* Album::createMutatorDelegate() {
		return provider->createAlbumMutatorDelegate(std::static_pointer_cast<Album>(self()));
	}
}

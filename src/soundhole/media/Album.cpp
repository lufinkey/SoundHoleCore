//
//  Album.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "Album.hpp"

namespace sh {
	$<AlbumItem> AlbumItem::new$($<Album> album, Data data) {
		return AlbumItem::new$(std::static_pointer_cast<SpecialTrackCollection<AlbumItem>>(album), data);
	}

	$<AlbumItem> AlbumItem::new$($<SpecialTrackCollection<AlbumItem>> album, Data data) {
		return fgl::new$<AlbumItem>(album, data);
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

	bool AlbumItem::matchesItem(const TrackCollectionItem* item) const {
		if(_track->trackNumber() == item->track()->trackNumber() && _track->uri() == item->track()->uri()) {
			return true;
		}
		return false;
	}
}

//
//  Playlist.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "Playlist.hpp"

namespace sh {
	$<PlaylistItem> PlaylistItem::new$($<Playlist> playlist, Data data) {
		return fgl::new$<PlaylistItem>(playlist, data);
	}

	$<PlaylistItem> PlaylistItem::new$($<SpecialTrackCollection<PlaylistItem>> playlist, Data data) {
		return fgl::new$<PlaylistItem>(std::static_pointer_cast<Playlist>(playlist), data);
	}

	PlaylistItem::PlaylistItem($<Playlist> playlist, Data data)
	: SpecialTrackCollectionItem<Playlist>(std::static_pointer_cast<TrackCollection>(playlist), data),
	_addedAt(data.addedAt), _addedBy(data.addedBy) {
		//
	}

	time_t PlaylistItem::addedAt() const {
		return _addedAt;
	}

	$<UserAccount> PlaylistItem::addedBy() {
		return _addedBy;
	}

	$<const UserAccount> PlaylistItem::addedBy() const {
		return std::static_pointer_cast<const UserAccount>(_addedBy);
	}

	bool PlaylistItem::matchesItem(const TrackCollectionItem* item) const {
		auto playlistItem = dynamic_cast<const PlaylistItem*>(item);
		if(playlistItem == nullptr) {
			return false;
		}
		if(_track->uri() == playlistItem->_track->uri()
		   && _addedAt == playlistItem->_addedAt && _addedBy->uri() == playlistItem->_addedBy->uri()) {
			return true;
		}
		return false;
	}
}

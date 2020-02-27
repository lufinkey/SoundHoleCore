//
//  Playlist.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "Playlist.hpp"
#include "MediaProvider.hpp"

namespace sh {
	$<PlaylistItem> PlaylistItem::new$($<SpecialTrackCollection<PlaylistItem>> playlist, Data data) {
		$<TrackCollectionItem> ptr;
		new PlaylistItem(ptr, playlist, data);
		return std::static_pointer_cast<PlaylistItem>(ptr);
	}

	PlaylistItem::PlaylistItem($<TrackCollectionItem>& ptr, $<SpecialTrackCollection<PlaylistItem>> playlist, Data data)
	: SpecialTrackCollectionItem<Playlist>(ptr, playlist, data),
	_addedAt(data.addedAt), _addedBy(data.addedBy) {
		//
	}

	const String& PlaylistItem::addedAt() const {
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

	PlaylistItem::Data PlaylistItem::toData() const {
		return PlaylistItem::Data{
			SpecialTrackCollectionItem<Playlist>::toData(),
			.addedAt=_addedAt,
			.addedBy=_addedBy
		};
	}

	$<PlaylistItem> PlaylistItem::fromJson($<SpecialTrackCollection<PlaylistItem>> playlist, Json json, MediaProviderStash* stash) {
		$<TrackCollectionItem> ptr;
		new PlaylistItem(ptr, playlist, json, stash);
		return std::static_pointer_cast<PlaylistItem>(ptr);
	}

	PlaylistItem::PlaylistItem($<TrackCollectionItem>& ptr, $<SpecialTrackCollection<PlaylistItem>> playlist, Json json, MediaProviderStash* stash)
	: SpecialTrackCollectionItem<Playlist>(ptr, playlist, json, stash) {
		auto addedBy = json["addedBy"];
		_addedBy = (!addedBy.is_null()) ? UserAccount::fromJson(addedBy, stash) : nullptr;
		_addedAt = json["addedAt"].string_value();
	}

	Json PlaylistItem::toJson() const {
		auto json = SpecialTrackCollectionItem<Playlist>::toJson().object_items();
		json.merge(Json::object{
			{ "addedBy", _addedBy ? _addedBy->toJson() : Json() },
			{ "addedAt", (std::string)_addedAt }
		});
		return json;
	}



	$<Playlist> Playlist::new$(MediaProvider* provider, Data data) {
		$<MediaItem> ptr;
		new Playlist(ptr, provider, data);
		return std::static_pointer_cast<Playlist>(ptr);
	}
	
	Playlist::Playlist($<MediaItem>& ptr, MediaProvider* provider, Data data)
	: SpecialTrackCollection<PlaylistItem>(ptr, provider, data),
	_owner(data.owner) {
		//
	}

	$<UserAccount> Playlist::owner() {
		return _owner;
	}

	$<const UserAccount> Playlist::owner() const {
		return std::static_pointer_cast<const UserAccount>(_owner);
	}

	bool Playlist::needsData() const {
		return tracksAreEmpty();
	}

	Promise<void> Playlist::fetchMissingData() {
		return provider->getPlaylistData(_uri).then([=](Data data) {
			if(tracksAreEmpty()) {
				_items = constructItems(data.tracks);
			}
		});
	}

	Playlist::Data Playlist::toData(DataOptions options) const {
		return Playlist::Data{
			SpecialTrackCollection<PlaylistItem>::toData(options),
			.owner=_owner
		};
	}

	$<Playlist> Playlist::fromJson(Json json, MediaProviderStash* stash) {
		$<MediaItem> ptr;
		new Playlist(ptr, json, stash);
		return std::static_pointer_cast<Playlist>(ptr);
	}

	Playlist::Playlist($<MediaItem>& ptr, Json json, MediaProviderStash* stash)
	: SpecialTrackCollection<PlaylistItem>(ptr, json, stash) {
		auto owner = json["owner"];
		_owner = (!owner.is_null()) ? UserAccount::fromJson(owner, stash) : nullptr;
	}

	Json Playlist::toJson(ToJsonOptions options) const {
		auto json = SpecialTrackCollection<PlaylistItem>::toJson(options).object_items();
		json.merge(Json::object{
			{ "owner", _owner ? _owner->toJson() : Json() }
		});
		return json;
	}

	Playlist::MutatorDelegate* Playlist::createMutatorDelegate() {
		return provider->createPlaylistMutatorDelegate(std::static_pointer_cast<Playlist>(self()));
	}
}

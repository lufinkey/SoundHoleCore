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
		return fgl::new$<PlaylistItem>(playlist, data);
	}

	PlaylistItem::PlaylistItem($<SpecialTrackCollection<PlaylistItem>> playlist, Data data)
	: SpecialTrackCollectionItem<Playlist>(playlist, data),
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
		return fgl::new$<PlaylistItem>(playlist, json, stash);
	}

	PlaylistItem::PlaylistItem($<SpecialTrackCollection<PlaylistItem>> playlist, Json json, MediaProviderStash* stash)
	: SpecialTrackCollectionItem<Playlist>(playlist, json, stash) {
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
		auto playlist = fgl::new$<Playlist>(provider, data);
		playlist->lazyLoadContentIfNeeded();
		return playlist;
	}
	
	Playlist::Playlist(MediaProvider* provider, Data data)
	: SpecialTrackCollection<PlaylistItem>(provider, data),
	_owner(data.owner) {
		//
	}

	$<UserAccount> Playlist::owner() {
		return _owner;
	}

	$<const UserAccount> Playlist::owner() const {
		return std::static_pointer_cast<const UserAccount>(_owner);
	}

	Promise<void> Playlist::fetchData() {
		auto self = std::static_pointer_cast<Playlist>(shared_from_this());
		return provider->getPlaylistData(_uri).then([=](Data data) {
			self->applyData(data);
		});
	}

	void Playlist::applyData(const Data& data) {
		SpecialTrackCollection<PlaylistItem>::applyData(data);
		if(Optional<String>(_owner ? maybe(_owner->uri()) : std::nullopt) != Optional<String>(data.owner ? maybe(data.owner->uri()) : std::nullopt)) {
			_owner = data.owner;
		}
	}

	Playlist::Data Playlist::toData(DataOptions options) const {
		return Playlist::Data{
			SpecialTrackCollection<PlaylistItem>::toData(options),
			.owner=_owner
		};
	}

	$<Playlist> Playlist::fromJson(Json json, MediaProviderStash* stash) {
		auto playlist = fgl::new$<Playlist>(json, stash);
		playlist->lazyLoadContentIfNeeded();
		return playlist;
	}

	Playlist::Playlist(Json json, MediaProviderStash* stash)
	: SpecialTrackCollection<PlaylistItem>(json, stash) {
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
		return provider->createPlaylistMutatorDelegate(std::static_pointer_cast<Playlist>(shared_from_this()));
	}
}

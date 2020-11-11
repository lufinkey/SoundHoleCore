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
	$<PlaylistItem> PlaylistItem::new$($<SpecialTrackCollection<PlaylistItem>> playlist, const Data& data) {
		return fgl::new$<PlaylistItem>(playlist, data);
	}

	PlaylistItem::PlaylistItem($<SpecialTrackCollection<PlaylistItem>> playlist, const Data& data)
	: SpecialTrackCollectionItem<Playlist>(playlist, data),
	_uniqueId(data.uniqueId), _addedAt(data.addedAt), _addedBy(data.addedBy) {
		//
	}

	const String& PlaylistItem::uniqueId() const {
		return _uniqueId;
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
		   && _uniqueId == playlistItem->_uniqueId
		   && _addedAt == playlistItem->_addedAt
		   && ((!_addedBy && !playlistItem->_addedBy) || (_addedBy && playlistItem->_addedBy && _addedBy->uri() == playlistItem->_addedBy->uri()))) {
			return true;
		}
		return false;
	}

	PlaylistItem::Data PlaylistItem::toData() const {
		return PlaylistItem::Data{
			SpecialTrackCollectionItem<Playlist>::toData(),
			.uniqueId=_uniqueId,
			.addedAt=_addedAt,
			.addedBy=_addedBy
		};
	}

	$<PlaylistItem> PlaylistItem::fromJson($<SpecialTrackCollection<PlaylistItem>> playlist, const Json& json, MediaProviderStash* stash) {
		return fgl::new$<PlaylistItem>(playlist, json, stash);
	}

	PlaylistItem::PlaylistItem($<SpecialTrackCollection<PlaylistItem>> playlist, const Json& json, MediaProviderStash* stash)
	: SpecialTrackCollectionItem<Playlist>(playlist, json, stash) {
		_uniqueId = json["uniqueId"].string_value();
		auto addedBy = json["addedBy"];
		_addedBy = (!addedBy.is_null()) ? UserAccount::fromJson(addedBy, stash) : nullptr;
		_addedAt = json["addedAt"].string_value();
	}

	Json PlaylistItem::toJson() const {
		auto json = SpecialTrackCollectionItem<Playlist>::toJson().object_items();
		json.merge(Json::object{
			{ "uniqueId", (std::string)_uniqueId },
			{ "addedBy", _addedBy ? _addedBy->toJson() : Json() },
			{ "addedAt", (std::string)_addedAt }
		});
		return json;
	}



	String Playlist::Privacy_toString(Privacy privacy) {
		switch(privacy) {
			case Privacy::PRIVATE:
				return "private";
			case Privacy::PUBLIC:
				return "public";
			case Privacy::UNLISTED:
				return "unlisted";
			case Privacy::UNKNOWN:
				return "unknown";
		}
		throw std::runtime_error("unknown privacy type");
	}

	Playlist::Privacy Playlist::Privacy_fromString(const String& privacyString) {
		if(privacyString == "private") {
			return Privacy::PRIVATE;
		} else if(privacyString == "public") {
			return Privacy::PUBLIC;
		} else if(privacyString == "unlisted") {
			return Privacy::UNLISTED;
		} else {
			return Privacy::UNKNOWN;
		}
	}

	$<Playlist> Playlist::new$(MediaProvider* provider, const Data& data) {
		auto playlist = fgl::new$<Playlist>(provider, data);
		playlist->lazyLoadContentIfNeeded();
		return playlist;
	}
	
	Playlist::Playlist(MediaProvider* provider, const Data& data)
	: SpecialTrackCollection<PlaylistItem>(provider, data),
	_owner(data.owner), _privacy(data.privacy) {
		//
	}

	$<UserAccount> Playlist::owner() {
		return _owner;
	}

	$<const UserAccount> Playlist::owner() const {
		return std::static_pointer_cast<const UserAccount>(_owner);
	}

	Playlist::Privacy Playlist::privacy() const {
		return _privacy;
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
			_privacy = data.privacy;
		}
	}

	Playlist::Data Playlist::toData(const DataOptions& options) const {
		return Playlist::Data{
			SpecialTrackCollection<PlaylistItem>::toData(options),
			.owner=_owner,
			.privacy=_privacy
		};
	}

	$<Playlist> Playlist::fromJson(const Json& json, MediaProviderStash* stash) {
		auto playlist = fgl::new$<Playlist>(json, stash);
		playlist->lazyLoadContentIfNeeded();
		return playlist;
	}

	Playlist::Playlist(const Json& json, MediaProviderStash* stash)
	: SpecialTrackCollection<PlaylistItem>(json, stash) {
		auto owner = json["owner"];
		_owner = (!owner.is_null()) ? UserAccount::fromJson(owner, stash) : nullptr;
		auto privacy = json["privacy"];
		if(!privacy.is_string()) {
			throw std::logic_error("missing required property privacy");
		}
		_privacy = Privacy_fromString(privacy.string_value());
	}

	Json Playlist::toJson(const ToJsonOptions& options) const {
		auto json = SpecialTrackCollection<PlaylistItem>::toJson(options).object_items();
		json.merge(Json::object{
			{ "owner", _owner ? _owner->toJson() : Json() },
			{ "privacy", (std::string)Privacy_toString(_privacy) }
		});
		return json;
	}

	Playlist::MutatorDelegate* Playlist::createMutatorDelegate() {
		return provider->createPlaylistMutatorDelegate(std::static_pointer_cast<Playlist>(shared_from_this()));
	}



	Promise<void> Playlist::insertItems(size_t index, LinkedList<$<Track>> items) {
		makeTracksAsync();
		return asyncItemsList()->insertItems(index, items);
	}

	Promise<void> Playlist::appendItems(LinkedList<$<Track>> items) {
		makeTracksAsync();
		return asyncItemsList()->appendItems(items);
	}

	Promise<void> Playlist::removeItems(size_t index, size_t count) {
		makeTracksAsync();
		return asyncItemsList()->removeItems(index, count);
	}

	Promise<void> Playlist::moveItems(size_t index, size_t count, size_t newIndex) {
		makeTracksAsync();
		return asyncItemsList()->moveItems(index, count, newIndex);
	}
}

//
//  Playlist.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "Playlist.hpp"
#include "MediaProvider.hpp"
#include <soundhole/database/MediaDatabase.hpp>

namespace sh {

	#pragma mark PlaylistItem::Data

	PlaylistItem::Data PlaylistItem::Data::fromJson(const Json& json, MediaProviderStash* stash) {
		auto collectionItemData = SpecialTrackCollectionItem<Playlist>::Data::fromJson(json, stash);
		auto addedByJson = json["addedBy"];
		$<UserAccount> addedBy;
		if(!addedByJson.is_null()) {
			auto mediaItem = stash->parseMediaItem(addedByJson);
			if(mediaItem) {
				addedBy = std::dynamic_pointer_cast<UserAccount>(mediaItem);
				if(!addedBy) {
					throw std::invalid_argument("Invalid json for PlaylistItem: parsed "+mediaItem->type()+" instead of expected type user in 'addedBy'");
				}
			}
		}
		return PlaylistItem::Data{
			collectionItemData,
			.uniqueId = json["uniqueId"].string_value(),
			.addedAt = Date::maybeFromISOString(json["addedAt"].string_value()).valueOr(Date::epoch()),
			.addedBy = addedBy
		};
	}



	#pragma mark PlaylistItem

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

	const Date& PlaylistItem::addedAt() const {
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

	Json PlaylistItem::toJson() const {
		auto json = SpecialTrackCollectionItem<Playlist>::toJson().object_items();
		json.merge(Json::object{
			{ "uniqueId", (std::string)_uniqueId },
			{ "addedBy", _addedBy ? _addedBy->toJson() : Json() },
			{ "addedAt", (std::string)_addedAt.toISOString() }
		});
		return json;
	}



	#pragma mark Playlist::Privacy

	String Playlist::Privacy_toString(Privacy privacy) {
		switch(privacy) {
			case Privacy::PRIVATE:
				return "private";
			case Privacy::PUBLIC:
				return "public";
			case Privacy::UNLISTED:
				return "unlisted";
		}
		throw std::runtime_error("unknown privacy type");
	}

	Optional<Playlist::Privacy> Playlist::Privacy_fromString(const String& privacyString) {
		if(privacyString == "private") {
			return Privacy::PRIVATE;
		} else if(privacyString == "public") {
			return Privacy::PUBLIC;
		} else if(privacyString == "unlisted") {
			return Privacy::UNLISTED;
		} else {
			return std::nullopt;
		}
	}



	#pragma mark Playlist::Data

	Playlist::Data Playlist::Data::fromJson(const Json& json, MediaProviderStash* stash) {
		auto collectionData = SpecialTrackCollection<PlaylistItem>::Data::fromJson(json, stash);
		auto ownerJson = json["owner"];
		$<UserAccount> owner;
		if(!ownerJson.is_null()) {
			auto mediaItem = stash->parseMediaItem(ownerJson);
			if(mediaItem) {
				owner = std::dynamic_pointer_cast<UserAccount>(mediaItem);
				if(!owner) {
					throw std::invalid_argument("Invalid json for Playlist: parsed "+mediaItem->type()+" instead of expected type user in 'owner'");
				}
			}
		}
		auto privacyJson = json["privacy"];
		if(!privacyJson.is_string()) {
			throw std::logic_error("missing required property privacy");
		}
		return Playlist::Data{
			collectionData,
			.owner = owner,
			.privacy = privacyJson.is_null() ? std::nullopt : Privacy_fromString(privacyJson.string_value())
		};
	}



	#pragma mark InsertItemOptions

	Playlist::InsertItemOptions Playlist::InsertItemOptions::defaultOptions() {
		return InsertItemOptions();
	}

	Map<String,Any> Playlist::InsertItemOptions::toMap() const {
		return {
			{ "database", database }
		};
	}

	Playlist::InsertItemOptions Playlist::InsertItemOptions::fromMap(const Map<String,Any>& dict) {
		auto database = dict.get("database", Any()).maybeAs<MediaDatabase*>().valueOr(nullptr);
		return InsertItemOptions{
			.database = database
		};
	}



	#pragma mark Playlist

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

	const Optional<Playlist::Privacy>& Playlist::privacy() const {
		return _privacy;
	}

	Optional<bool> Playlist::editable() const {
		return _editable;
	}

	Promise<bool> Playlist::isEditable() {
		auto self = std::static_pointer_cast<Playlist>(shared_from_this());
		return provider->isPlaylistEditable(self).map([=](bool editable) -> bool {
			self->_editable = editable;
			return editable;
		});
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

	Json Playlist::toJson(const ToJsonOptions& options) const {
		auto json = SpecialTrackCollection<PlaylistItem>::toJson(options).object_items();
		json.merge(Json::object{
			{ "owner", _owner ? _owner->toJson() : Json() },
			{ "privacy", _privacy ? (std::string)Privacy_toString(_privacy.value()) : Json() }
		});
		return json;
	}

	Promise<void> Playlist::insertAsyncListItems(typename AsyncList::Mutator* mutator, size_t index, LinkedList<$<Track>> tracks, Map<String,Any> options) {
		auto delegate = mutatorDelegate();
		return delegate->insertItems(mutator, index, tracks, InsertItemOptions::fromMap(options));
	}

	Promise<void> Playlist::appendAsyncListItems(typename AsyncList::Mutator* mutator, LinkedList<$<Track>> tracks, Map<String,Any> options) {
		auto delegate = mutatorDelegate();
		return delegate->appendItems(mutator, tracks, InsertItemOptions::fromMap(options));
	}

	Promise<void> Playlist::removeAsyncListItems(typename AsyncList::Mutator* mutator, size_t index, size_t count, Map<String,Any> options) {
		auto delegate = mutatorDelegate();
		return delegate->removeItems(mutator, index, count);
	}

	Promise<void> Playlist::moveAsyncListItems(typename AsyncList::Mutator* mutator, size_t index, size_t count, size_t newIndex, Map<String,Any> options) {
		auto delegate = mutatorDelegate();
		return delegate->moveItems(mutator, index, count, newIndex);
	}

	Playlist::MutatorDelegate* Playlist::createMutatorDelegate() {
		return provider->createPlaylistMutatorDelegate(std::static_pointer_cast<Playlist>(shared_from_this()));
	}



	bool Playlist::canInsertItem($<Track> item) const {
		auto delegate = const_cast<Playlist*>(this)->mutatorDelegate();
		return delegate->canInsertItem(item);
	}

	bool Playlist::canInsertItems(const LinkedList<$<Track>>& items) const {
		auto delegate = const_cast<Playlist*>(this)->mutatorDelegate();
		return !items.containsWhere([&](auto& item) { return !delegate->canInsertItem(item); });
	}

	Promise<void> Playlist::insertItems(size_t index, LinkedList<$<Track>> items, InsertItemOptions options) {
		if(!canInsertItems(items)) {
			return rejectWith(std::invalid_argument("Cannot insert one or more of the given items"));
		}
		makeTracksAsync();
		return asyncItemsList()->insertItems(index, items, options.toMap());
	}

	Promise<void> Playlist::appendItems(LinkedList<$<Track>> items, InsertItemOptions options) {
		if(!canInsertItems(items)) {
			return rejectWith(std::invalid_argument("Cannot append one or more of the given items"));
		}
		makeTracksAsync();
		return asyncItemsList()->appendItems(items, options.toMap());
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

//
//  UserAccount.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/25/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "UserAccount.hpp"
#include "MediaProvider.hpp"

namespace sh {
	$<UserAccount> UserAccount::new$(MediaProvider* provider, const Data& data) {
		$<MediaItem> ptr;
		new UserAccount(provider, data);
		return std::static_pointer_cast<UserAccount>(ptr);
	}

	UserAccount::UserAccount(MediaProvider* provider, const Data& data)
	: MediaItem(provider, data),
	_id(data.id), _displayName(data.displayName) {
		//
	}

	const String& UserAccount::id() const {
		return _id;
	}

	const Optional<String>& UserAccount::displayName() const {
		return _displayName;
	}

	Promise<void> UserAccount::fetchData() {
		auto self = std::static_pointer_cast<UserAccount>(shared_from_this());
		return provider->getUserData(_uri).then([=](Data data) {
			self->applyData(data);
		});
	}

	void UserAccount::applyData(const Data& data) {
		MediaItem::applyData(data);
		_id = data.id;
		if(data.displayName) {
			_displayName = data.displayName;
		}
	}

	UserAccount::Data UserAccount::toData() const {
		return UserAccount::Data{
			MediaItem::toData(),
			.id=_id,
			.displayName=_displayName
		};
	}



	$<UserAccount> UserAccount::fromJson(const Json& json, MediaProviderStash* stash) {
		return fgl::new$<UserAccount>(json, stash);
	}

	UserAccount::UserAccount(const Json& json, MediaProviderStash* stash)
	: MediaItem(json, stash) {
		auto id = json["id"];
		auto displayName = json["displayName"];
		if(!id.is_string() || (!displayName.is_null() && !displayName.is_string())) {
			throw std::invalid_argument("invalid json for UserAccount");
		}
		_id = id.string_value();
		_displayName = (!displayName.is_null()) ? maybe((String)displayName.string_value()) : std::nullopt;
	}

	Json UserAccount::toJson() const {
		auto json = MediaItem::toJson().object_items();
		json.merge(Json::object{
			{"id",(std::string)_id},
			{"displayName",(_displayName ? Json((std::string)_displayName.value()) : Json())}
		});
		return json;
	}
}

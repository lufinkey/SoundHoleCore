//
//  UserAccount.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/25/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "UserAccount.hpp"

namespace sh {
	$<UserAccount> UserAccount::new$(MediaProvider* provider, Data data) {
		$<MediaItem> ptr;
		new UserAccount(ptr, provider, data);
		return std::static_pointer_cast<UserAccount>(ptr);
	}

	UserAccount::UserAccount($<MediaItem>& ptr, MediaProvider* provider, Data data)
	: MediaItem(ptr, provider, data),
	_id(data.id), _displayName(data.displayName) {
		//
	}

	const String& UserAccount::id() const {
		return _id;
	}

	const Optional<String>& UserAccount::displayName() const {
		return _displayName;
	}

	bool UserAccount::needsData() const {
		// TODO implement needsData
		return false;
	}

	Promise<void> UserAccount::fetchMissingData() {
		// TODO implement fetchMissingData
		return Promise<void>::reject(std::runtime_error("not implemented"));
	}

	UserAccount::Data UserAccount::toData() const {
		return UserAccount::Data{
			MediaItem::toData(),
			.id=_id,
			.displayName=_displayName
		};
	}



	$<UserAccount> UserAccount::fromJson(Json json, FromJsonOptions options) {
		$<MediaItem> ptr;
		new UserAccount(ptr, json, options);
		return std::static_pointer_cast<UserAccount>(ptr);
	}

	UserAccount::UserAccount($<MediaItem>& ptr, Json json, FromJsonOptions options)
	: MediaItem(ptr, json, options) {
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

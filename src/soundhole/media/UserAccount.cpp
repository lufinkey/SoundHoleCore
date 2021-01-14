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

	#pragma mark UserAccount::Data

	UserAccount::Data UserAccount::Data::fromJson(const Json& json, MediaProviderStash* stash) {
		auto mediaItemData = MediaItem::Data::fromJson(json, stash);
		auto idJson = json["id"];
		auto displayNameJson = json["displayName"];
		if(!idJson.is_string() || (!displayNameJson.is_null() && !displayNameJson.is_string())) {
			throw std::invalid_argument("invalid json for UserAccount");
		}
		return UserAccount::Data{
			mediaItemData,
			.id = idJson.string_value(),
			.displayName = (!displayNameJson.is_null()) ? maybe((String)displayNameJson.string_value()) : std::nullopt
		};
	}



	#pragma mark UserAccount

	$<UserAccount> UserAccount::new$(MediaProvider* provider, const Data& data) {
		return fgl::new$<UserAccount>(provider, data);
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

	Json UserAccount::toJson() const {
		auto json = MediaItem::toJson().object_items();
		json.merge(Json::object{
			{"id",(std::string)_id},
			{"displayName",(_displayName ? Json((std::string)_displayName.value()) : Json())}
		});
		return json;
	}
}

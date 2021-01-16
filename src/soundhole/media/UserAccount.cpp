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

	#pragma mark UserAccount

	$<UserAccount> UserAccount::new$(MediaProvider* provider, const Data& data) {
		return fgl::new$<UserAccount>(provider, data);
	}

	UserAccount::UserAccount(MediaProvider* provider, const Data& data)
	: MediaItem(provider, data) {
		//
	}

	Promise<void> UserAccount::fetchData() {
		auto self = std::static_pointer_cast<UserAccount>(shared_from_this());
		return provider->getUserData(_uri).then([=](Data data) {
			self->applyData(data);
		});
	}

	void UserAccount::applyData(const Data& data) {
		MediaItem::applyData(data);
	}

	UserAccount::Data UserAccount::toData() const {
		return UserAccount::Data{
			MediaItem::toData()
		};
	}

	Json UserAccount::toJson() const {
		auto json = MediaItem::toJson().object_items();
		return json;
	}
}

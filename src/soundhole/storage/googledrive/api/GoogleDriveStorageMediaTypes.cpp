//
//  GoogleDriveStorageMediaTypes.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/23/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "GoogleDriveStorageMediaTypes.hpp"
#include <soundhole/utils/js/JSUtils.hpp>

namespace sh {
	Json GoogleDriveStorageProviderUser::toJson() const {
		return Json::object{
			{ "uri", (std::string)uri },
			{ "kind", (std::string)kind },
			{ "displayName", (std::string)displayName },
			{ "photoLink", (std::string)photoLink },
			{ "permissionId", (std::string)permissionId },
			{ "emailAddress", (std::string)emailAddress },
			{ "me", me },
			{ "baseFolderId", baseFolderId.empty() ? Json() : Json((std::string)baseFolderId) }
		};
	}

	GoogleDriveStorageProviderUser GoogleDriveStorageProviderUser::fromJson(const Json& json) {
		return GoogleDriveStorageProviderUser{
			.uri = json["uri"].string_value(),
			.kind = json["kind"].string_value(),
			.displayName = json["displayName"].string_value(),
			.photoLink = json["photoLink"].string_value(),
			.permissionId = json["permissionId"].string_value(),
			.emailAddress = json["emailAddress"].string_value(),
			.me = json["me"].bool_value(),
			.baseFolderId = json["baseFolderId"].string_value()
		};
	}

	#ifdef NODE_API_MODULE
	GoogleDriveStorageProviderUser GoogleDriveStorageProviderUser::fromNapiObject(Napi::Object obj) {
		return GoogleDriveStorageProviderUser{
			.uri = jsutils::nonNullStringPropFromNapiObject(obj, "uri"),
			.kind = jsutils::nonNullStringPropFromNapiObject(obj, "kind"),
			.displayName = jsutils::nonNullStringPropFromNapiObject(obj, "displayName"),
			.photoLink = jsutils::nonNullStringPropFromNapiObject(obj, "photoLink"),
			.permissionId = jsutils::nonNullStringPropFromNapiObject(obj, "permissionId"),
			.emailAddress = jsutils::nonNullStringPropFromNapiObject(obj, "emailAddress"),
			.me = obj.Get("me").ToBoolean().Value(),
			.baseFolderId = jsutils::stringFromNapiValue(obj.Get("baseFolderId"))
		};
	}
	#endif



	GoogleDrivePlaylistItemsPage GoogleDrivePlaylistItemsPage::fromJson(const Json& json, MediaProviderStash* stash) {
		return GoogleDrivePlaylistItemsPage{
			.offset = (size_t)json["offset"].number_value(),
			.total = (size_t)json["total"].number_value(),
			.items = jsutils::arrayListFromJson<PlaylistItem::Data>(json["items"], [=](const Json& itemJson) {
				return PlaylistItem::Data::fromJson(itemJson, stash);
			})
		};
	}
}

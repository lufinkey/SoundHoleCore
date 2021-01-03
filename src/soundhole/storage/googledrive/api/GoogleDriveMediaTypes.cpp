//
//  GoogleDriveMediaTypes.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/2/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include <napi.h>
#include "GoogleDriveMediaTypes.hpp"
#include <soundhole/utils/js/JSUtils.hpp>

namespace sh {
	Json GoogleDriveUser::toJson() const {
		return Json::object{
			{ "kind", (std::string)kind },
			{ "displayName", (std::string)displayName },
			{ "photoLink", (std::string)photoLink },
			{ "permissionId", (std::string)permissionId },
			{ "emailAddress", (std::string)emailAddress },
			{ "me", me }
		};
	}

	GoogleDriveUser GoogleDriveUser::fromJson(const Json& json) {
		return GoogleDriveUser{
			.kind=json["kind"].string_value(),
			.displayName=json["displayName"].string_value(),
			.photoLink=json["photoLink"].string_value(),
			.permissionId=json["permissionId"].string_value(),
			.emailAddress=json["emailAddress"].string_value(),
			.me=json["me"].bool_value()
		};
	}

	#ifdef NODE_API_MODULE
	GoogleDriveUser GoogleDriveUser::fromNapiObject(Napi::Object obj) {
		return GoogleDriveUser{
			.kind=jsutils::nonNullStringPropFromNapiObject(obj, "kind"),
			.displayName=jsutils::nonNullStringPropFromNapiObject(obj, "displayName"),
			.photoLink=jsutils::nonNullStringPropFromNapiObject(obj, "photoLink"),
			.permissionId=jsutils::nonNullStringPropFromNapiObject(obj, "permissionId"),
			.emailAddress=jsutils::nonNullStringPropFromNapiObject(obj, "emailAddress"),
			.me=obj.Get("me").ToBoolean().Value()
		};
	}
	#endif
}

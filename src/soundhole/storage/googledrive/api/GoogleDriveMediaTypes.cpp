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
}

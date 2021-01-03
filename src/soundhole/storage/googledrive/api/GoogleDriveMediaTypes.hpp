//
//  GoogleDriveMediaTypes.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/2/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	struct GoogleDriveUser {
		String kind;
		String displayName;
		String photoLink;
		String permissionId;
		String emailAddress;
		bool me;
		
		Json toJson() const;
		
		static GoogleDriveUser fromJson(const Json&);
		#ifdef NODE_API_MODULE
		static GoogleDriveUser fromNapiObject(Napi::Object);
		#endif
	};
}

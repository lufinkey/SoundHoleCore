//
//  Utils.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "Utils.hpp"
#include <httplib.h>

namespace sh::utils {
	String makeQueryString(std::map<String,String> params) {
		LinkedList<String> items;
		for(auto& pair : params) {
			items.pushBack(
				httplib::detail::encode_url(pair.first) + "=" + httplib::detail::encode_url(pair.second)
			);
		}
		return String::join(items, "&");
	}
}

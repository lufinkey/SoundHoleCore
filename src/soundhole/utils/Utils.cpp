//
//  Utils.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/26/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "Utils.hpp"
#include <time.h>

namespace sh::utils {
	time_t stringToTime(const String& str) {
		struct tm tm;
		strptime(str.c_str(), "%Y-%m-%jT%H:%M:%S%z", &tm);
		return std::mktime(&tm);
	}
}

//
//  Base64.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/13/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh::utils {
	String base64_encode(const String& str);
	String base64_encode(const unsigned char* bytes, size_t length);

	String base64_decode(const String& str);
}

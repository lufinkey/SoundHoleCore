//
//  SecureStore.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/5/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once
#include <soundhole/common.hpp>

namespace sh {
	namespace SecureStore {
		void setSecureData(const String& key, const Data& data);
		Data getSecureData(const String& key);
		void deleteSecureData(const String& key);
	};
}

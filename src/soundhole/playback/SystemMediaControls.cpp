//
//  SystemMediaControls.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/23/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "SystemMediaControls.hpp"

namespace sh {
	SystemMediaControls* _sharedSystemMediaControls = nullptr;

	SystemMediaControls* SystemMediaControls::shared() {
		if(_sharedSystemMediaControls == nullptr) {
			_sharedSystemMediaControls = new SystemMediaControls();
		}
		return _sharedSystemMediaControls;
	}
}

//
//  NamedProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/3/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class NamedProvider {
	public:
		virtual String name() const = 0;
		virtual String displayName() const = 0;
	};
}

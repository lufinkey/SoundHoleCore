//
//  SQLOrder.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 5/26/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh::sql {
	enum class Order {
		NONE,
		ASC,
		DESC
	};
}

//
//  SQLIndexRange.h
//  SoundHoleCore
//
//  Created by Luis Finke on 5/26/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh::sql {
	struct IndexRange {
		size_t startIndex;
		size_t endIndex;
	};
}

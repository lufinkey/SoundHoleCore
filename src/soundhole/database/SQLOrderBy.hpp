//
//  SQLOrderBy.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 7/3/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh::sql {
	enum class LibraryItemOrderBy {
		ADDED_AT,
		NAME
	};
	String LibraryItemOrderBy_toString(LibraryItemOrderBy);
	LibraryItemOrderBy LibraryItemOrderBy_fromString(String);
}

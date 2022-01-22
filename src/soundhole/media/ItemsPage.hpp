//
//  ItemsPage.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/17/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	template<typename T>
	struct ItemsPage {
		ArrayList<T> items;
		size_t total;
	};
}

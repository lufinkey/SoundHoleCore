//
//  Utils.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <map>

namespace sh::utils {
	String makeQueryString(std::map<String,String> params);
}

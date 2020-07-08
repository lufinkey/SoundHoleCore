//
//  common.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <memory>
#include <fgl/data.hpp>
#include <fgl/async.hpp>
#include <fgl/io.hpp>
#include <json11/json11.hpp>
#include <embed/nodejs/NAPI_Types.hpp>

#ifndef FGL_ASYNCLIST_USED_DTL
#error "SoundHoleCore requires use of DTL in AsyncList"
#endif

namespace sh {
	using namespace fgl;
	using Json = json11::Json;
}

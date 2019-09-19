//
//  Scripts.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/external.hpp>

struct napi_ref__;
typedef struct napi_ref__* napi_ref;

namespace sh::scripts {
	Promise<void> loadScriptsIfNeeded();
	napi_ref getJSExportsRef();
}

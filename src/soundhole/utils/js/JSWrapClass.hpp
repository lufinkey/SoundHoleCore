//
//  JSWrapClass.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh {
	class JSWrapClass {
	protected:
		virtual void initializeJS(napi_env) = 0;
		void queueJS(Function<void(napi_env)> work);
		void queueJSDestruct(Function<void(napi_env)> work);
	};
}

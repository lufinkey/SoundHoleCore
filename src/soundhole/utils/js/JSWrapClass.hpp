//
//  JSWrapClass.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/18/19.
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
		
		Json jsonFromNAPIValue(napi_env env, napi_value value);
		Promise<Json> jsonPromiseFromNAPIValue(napi_env env, napi_value value);
		
		#ifdef NODE_API_MODULE
		template<typename NapiType>
		NapiType jsValue(napi_env env, napi_ref ref) {
			if(ref == nullptr) {
				return NapiType();
			}
			return Napi::Reference<NapiType>(env, ref).Value();
		}
		#endif
	};
}

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
		
		Json jsonFromNapiValue(napi_env env, napi_value value) const;
		
		#ifdef NODE_API_MODULE
		
		Json jsonFromNapiValue(Napi::Value value) const;
		
		template<typename NapiType>
		NapiType jsValue(napi_env env, napi_ref ref) const {
			if(ref == nullptr) {
				return NapiType();
			}
			return Napi::Reference<NapiType>(env, ref).Value();
		}
		
		#endif
	};
}

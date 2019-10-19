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
		String stringFromNapiValue(Napi::Value value) const;
		
		template<typename T>
		ArrayList<T> arrayListFromNapiArray(Napi::Array array, Function<T(Napi::Value)> transform) const {
			ArrayList<T> newList;
			uint32_t listLength = array.Length();
			newList.reserve((size_t)listLength);
			for(uint32_t i=0; i<listLength; i++) {
				newList.pushBack(transform(array.Get(i)));
			}
			return newList;
		}
		
		template<typename T>
		LinkedList<T> linkedListFromNapiArray(Napi::Array array, Function<T(Napi::Value)> transform) const {
			LinkedList<T> newList;
			uint32_t listLength = array.Length();
			for(uint32_t i=0; i<listLength; i++) {
				newList.pushBack(transform(array.Get(i)));
			}
			return newList;
		}
		
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

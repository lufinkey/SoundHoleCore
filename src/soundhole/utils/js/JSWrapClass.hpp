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
	public:
		static Json jsonFromNapiValue(napi_env env, napi_value value);
		
		#ifdef NODE_API_MODULE
		
		static Json jsonFromNapiValue(Napi::Value value);
		static String stringFromNapiValue(Napi::Value value);
		static Optional<String> optStringFromNapiValue(Napi::Value value);
		
		template<typename T>
		static ArrayList<T> arrayListFromNapiArray(Napi::Array array, Function<T(Napi::Value)> transform) {
			if(array.IsEmpty() || array.IsNull() || array.IsUndefined()) {
				return {};
			}
			ArrayList<T> newList;
			uint32_t listLength = array.Length();
			newList.reserve((size_t)listLength);
			for(uint32_t i=0; i<listLength; i++) {
				newList.pushBack(transform(array.Get(i)));
			}
			return newList;
		}
		
		template<typename T>
		static Optional<ArrayList<T>> optArrayListFromNapiArray(Napi::Array array, Function<T(Napi::Value)> transform) {
			if(array.IsEmpty() || array.IsNull() || array.IsUndefined()) {
				return std::nullopt;
			}
			return arrayListFromNapiArray<T>(array, transform);
		}
		
		template<typename T>
		static LinkedList<T> linkedListFromNapiArray(Napi::Array array, Function<T(Napi::Value)> transform) {
			if(array.IsNull() || array.IsUndefined()) {
				return {};
			}
			LinkedList<T> newList;
			uint32_t listLength = array.Length();
			for(uint32_t i=0; i<listLength; i++) {
				newList.pushBack(transform(array.Get(i)));
			}
			return newList;
		}
		
		template<typename T>
		static Optional<LinkedList<T>> optLinkedListFromNapiArray(Napi::Array array, Function<T(Napi::Value)> transform) {
			if(array.IsEmpty() || array.IsNull() || array.IsUndefined()) {
				return std::nullopt;
			}
			return linkedListFromNapiArray<T>(array, transform);
		}
		
		template<typename NapiType>
		NapiType jsValue(napi_env env, napi_ref ref) const {
			if(ref == nullptr) {
				return NapiType();
			}
			return Napi::Reference<NapiType>(env, ref).Value();
		}
		
		#endif
		
	protected:
		virtual void initializeJS(napi_env) = 0;
		void queueJS(Function<void(napi_env)> work);
		void queueJSDestruct(Function<void(napi_env)> work);
	};
}

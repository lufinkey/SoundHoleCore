//
//  JSUtils.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh::jsutils {
	template<typename T>
	ArrayList<T> arrayListFromJson(const Json& json, const Function<T(const Json&)>& transform) {
		if(!json.is_array()) {
			return {};
		}
		ArrayList<T> list;
		list.reserve(json.array_items().size());
		for(auto& jsonItem : json.array_items()) {
			list.pushBack(transform(jsonItem));
		}
		return list;
	}
	
	template<typename T>
	Optional<ArrayList<T>> optArrayListFromJson(const Json& json, const Function<T(const Json&)>& transform) {
		if(json.is_null()) {
			return std::nullopt;
		}
		return arrayListFromJson(json, transform);
	}
	
	Json jsonFromNapiValue(napi_env env, napi_value value);
	
	#ifdef NODE_API_MODULE
	
	Json jsonFromNapiValue(Napi::Value value);
	String stringFromNapiValue(Napi::Value value);
	Optional<String> optStringFromNapiValue(Napi::Value value);
	size_t sizeFromNapiValue(Napi::Value value);
	Optional<size_t> optSizeFromNapiValue(Napi::Value value);
	double doubleFromNapiValue(Napi::Value value);
	Optional<double> optDoubleFromNapiValue(Napi::Value value);
	
	template<typename T>
	ArrayList<T> arrayListFromNapiArray(Napi::Array array, Function<T(Napi::Value)> transform) {
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
	ArrayList<T> arrayListFromNapiValue(Napi::Value array, Function<T(Napi::Value)> transform) {
		return arrayListFromNapiArray(array.As<Napi::Array>(), transform);
	}
	
	template<typename T>
	Optional<ArrayList<T>> optArrayListFromNapiArray(Napi::Array array, Function<T(Napi::Value)> transform) {
		if(array.IsEmpty() || array.IsNull() || array.IsUndefined()) {
			return std::nullopt;
		}
		return arrayListFromNapiArray<T>(array, transform);
	}
	
	template<typename T>
	LinkedList<T> linkedListFromNapiArray(Napi::Array array, Function<T(Napi::Value)> transform) {
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
	LinkedList<T> linkedListFromNapiValue(Napi::Value array, Function<T(Napi::Value)> transform) {
		return linkedListFromNapiArray(array.As<Napi::Array>(), transform);
	}
	
	template<typename T>
	Optional<LinkedList<T>> optLinkedListFromNapiArray(Napi::Array array, Function<T(Napi::Value)> transform) {
		if(array.IsEmpty() || array.IsNull() || array.IsUndefined()) {
			return std::nullopt;
		}
		return linkedListFromNapiArray<T>(array, transform);
	}
	
	template<typename NapiType>
	NapiType jsValue(napi_env env, napi_ref ref) {
		if(ref == nullptr) {
			return NapiType();
		}
		auto napiRef = Napi::Reference<NapiType>(env, ref);
		napiRef.SuppressDestruct();
		return napiRef.Value();
	}
	
	#endif
}

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
	template<typename Transform>
	auto arrayListFromJson(const Json& json, Transform transform) {
		using ReturnType = decltype(transform(std::declval<const Json>()));
		auto list = ArrayList<ReturnType>();
		if(!json.is_array()) {
			return list;
		}
		list.reserve(json.array_items().size());
		for(auto& jsonItem : json.array_items()) {
			list.pushBack(transform(jsonItem));
		}
		return list;
	}

	template<typename Transform>
	auto singleOrArrayListFromJson(const Json& json, Transform transform) {
		using ReturnType = decltype(transform(std::declval<const Json>()));
		auto list = ArrayList<ReturnType>();
		if(!json.is_array()) {
			if(!json.is_null()) {
				list.pushBack(transform(json));
			}
			return list;
		}
		list.reserve(json.array_items().size());
		for(auto& jsonItem : json.array_items()) {
			list.pushBack(transform(jsonItem));
		}
		return list;
	}
	
	template<typename Transform>
	auto optArrayListFromJson(const Json& json, Transform transform) {
		using ReturnType = decltype(transform(std::declval<const Json>()));
		if(json.is_null()) {
			return Optional<ArrayList<ReturnType>>();
		}
		return maybe(arrayListFromJson(json, transform));
	}

	template<typename Transform>
	auto optSingleOrArrayListFromJson(const Json& json, Transform transform) {
		using ReturnType = decltype(transform(std::declval<const Json>()));
		if(json.is_null()) {
			return Optional<ArrayList<ReturnType>>();
		}
		return maybe(singleOrArrayListFromJson(json, transform));
	}

	template<typename Transform>
	auto mapFromJson(const Json& json, Transform transform) {
		using ReturnType = decltype(transform(std::declval<const std::string>(), std::declval<const Json>()));
		std::map<String,ReturnType> map;
		for(auto& pair : json.object_items()) {
			map.insert_or_assign(pair.first, transform(pair.first,pair.second));
		}
		return map;
	}

	template<typename T, typename Transform>
	Json jsonFromMap(const std::map<String,T>& map, Transform transform) {
		Json::object json;
		for(auto& pair : map) {
			json[pair.first] = transform(pair.first,pair.second);
		}
		return json;
	}

	Optional<bool> optBoolFromJson(const Json&);
	Optional<size_t> optSizeFromJson(const Json&);
	Optional<double> optDoubleFromJson(const Json&);
	Optional<String> optStringFromJson(const Json&);
	String nonNullStringPropFromJson(const Json& json, const char* propName);
	size_t nonNullSizePropFromJson(const Json& json, const char* propName);

	Optional<size_t> badlyFormattedSizeFromJson(const Json& json);
	Optional<bool> badlyFormattedBoolFromJson(const Json& json);
	Optional<double> badlyFormattedDoubleFromJson(const Json& json);

    String stringFromJson(const Json& json);
	
	#ifdef NODE_API_MODULE

	Json jsonFromNapiValue(napi_env env, napi_value value);
	
	Json jsonFromNapiValue(Napi::Value value);
	String nonNullStringPropFromNapiObject(Napi::Object obj, const char* propName);
	String stringFromNapiValue(Napi::Value value);
	Optional<String> optStringFromNapiValue(Napi::Value value);
	size_t nonNullSizePropFromNapiObject(Napi::Object obj, const char* propName);
	size_t sizeFromNapiValue(Napi::Value value);
	Optional<size_t> optSizeFromNapiValue(Napi::Value value);
	double doubleFromNapiValue(Napi::Value value);
	Optional<double> optDoubleFromNapiValue(Napi::Value value);
	bool boolFromNapiValue(Napi::Value value);
	Optional<bool> optBoolFromNapiValue(Napi::Value value);

	template<typename Transform>
	inline auto optValueFromNapiValue(Napi::Value value, Transform transform) {
		using ReturnType = decltype(transform(std::declval<Napi::Value>()));
		if(value.IsEmpty() || value.IsNull() || value.IsUndefined()) {
			return Optional<ReturnType>();
		}
		return maybe(transform(value));
	}
	
	template<typename Transform>
	auto arrayListFromNapiArray(Napi::Array array, Transform transform) {
		using ReturnType = decltype(transform(std::declval<Napi::Value>()));
		auto newList = ArrayList<ReturnType>();
		if(array.IsEmpty() || array.IsNull() || array.IsUndefined()) {
			return newList;
		}
		uint32_t listLength = array.Length();
		newList.reserve((size_t)listLength);
		for(uint32_t i=0; i<listLength; i++) {
			newList.pushBack(transform(array.Get(i)));
		}
		return newList;
	}
	
	template<typename Transform>
	auto arrayListFromNapiValue(Napi::Value array, Transform transform) {
		return arrayListFromNapiArray(array.As<Napi::Array>(), transform);
	}
	
	template<typename Transform>
	auto optArrayListFromNapiArray(Napi::Array array, Transform transform) {
		using ReturnType = decltype(transform(std::declval<Napi::Value>()));
		if(array.IsEmpty() || array.IsNull() || array.IsUndefined()) {
			return Optional<ArrayList<ReturnType>>();
		}
		return maybe(arrayListFromNapiArray(array, transform));
	}

	template<typename Transform>
	auto optArrayListFromNapiValue(Napi::Value array, Transform transform) {
		using ReturnType = decltype(transform(std::declval<Napi::Value>()));
		if(array.IsEmpty() || array.IsNull() || array.IsUndefined()) {
			return Optional<ArrayList<ReturnType>>();
		}
		return maybe(arrayListFromNapiValue(array, transform));
	}
	
	template<typename Transform>
	auto linkedListFromNapiArray(Napi::Array array, Transform transform) {
		using ReturnType = decltype(transform(std::declval<Napi::Value>()));
		auto newList = LinkedList<ReturnType>();
		if(array.IsNull() || array.IsUndefined()) {
			return newList;
		}
		uint32_t listLength = array.Length();
		for(uint32_t i=0; i<listLength; i++) {
			newList.pushBack(transform(array.Get(i)));
		}
		return newList;
	}
	
	template<typename Transform>
	auto linkedListFromNapiValue(Napi::Value array, Transform transform) {
		return linkedListFromNapiArray(array.As<Napi::Array>(), transform);
	}
	
	template<typename Transform>
	auto optLinkedListFromNapiArray(Napi::Array array, Transform transform) {
		using ReturnType = decltype(transform(std::declval<Napi::Value>()));
		if(array.IsEmpty() || array.IsNull() || array.IsUndefined()) {
			return Optional<LinkedList<ReturnType>>();
		}
		return linkedListFromNapiArray(array, transform);
	}

	template<typename Transform>
	auto mapFromNapiObject(Napi::Object object, Transform transform) {
		using ReturnType = decltype(transform(std::declval<String>(), std::declval<Napi::Value>()));
		std::map<String,ReturnType> map;
		if(object.IsEmpty() || object.IsNull() || object.IsUndefined()) {
			return map;
		}
		Napi::Array keys = object.GetPropertyNames();
		for(uint32_t i=0, length=keys.Length(); i<length; i++) {
			Napi::Value propNameValue = keys[i];
			String propertyName = propNameValue.As<Napi::String>().Utf8Value();
			auto value = object.Get(propertyName.c_str());
			map[propertyName] = transform(propertyName,value);
		}
		return map;
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

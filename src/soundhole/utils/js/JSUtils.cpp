//
//  JSUtils.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include <napi.h>
#include "JSWrapClass.hpp"
#include <soundhole/scripts/Scripts.hpp>
#include <embed/nodejs/NodeJS.hpp>

namespace sh::jsutils {
	Json jsonFromNapiValue(napi_env env, napi_value value) {
		auto jsExports = scripts::getJSExports(env);
		auto resultJson = jsExports.Get("json_encode").As<Napi::Function>().Call({ value }).As<Napi::String>();
		std::string parseError;
		auto result = Json::parse(resultJson.Utf8Value(), parseError);
		if(!parseError.empty()) {
			throw std::runtime_error("Failed to convert napi_value to Json");
		}
		return result;
	}

	Json jsonFromNapiValue(Napi::Value value) {
		return jsonFromNapiValue(value.Env(), value);
	}

	String stringFromNapiValue(Napi::Value value) {
		if(value.IsEmpty() || value.IsNull() || value.IsUndefined()) {
			return "";
		}
		return value.ToString();
	}

	Optional<String> optStringFromNapiValue(Napi::Value value) {
		if(value.IsEmpty() || value.IsNull() || value.IsUndefined()) {
			return std::nullopt;
		}
		return String(value.ToString());
	}

	size_t sizeFromNapiValue(Napi::Value value) {
		if(value.IsEmpty() || value.IsNull() || value.IsUndefined()) {
			return 0;
		}
		return (size_t)value.As<Napi::Number>().Int64Value();
	}

	Optional<size_t> optSizeFromNapiValue(Napi::Value value) {
		if(value.IsEmpty() || value.IsNull() || value.IsUndefined()) {
			return std::nullopt;
		}
		return sizeFromNapiValue(value);
	}
}

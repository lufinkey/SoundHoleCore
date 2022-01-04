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
	Optional<bool> optBoolFromJson(const Json& json) {
		if(json.is_null()) {
			return std::nullopt;
		}
		return json.bool_value();
	}

	Optional<size_t> optSizeFromJson(const Json& json) {
		if(!json.is_number()) {
			return std::nullopt;
		}
		return (size_t)json.number_value();
	}

	Optional<size_t> badlyFormattedSizeFromJson(const Json& json) {
		if(json.is_number()) {
			if(json.number_value() < 0) {
				return std::nullopt;
			}
			return (size_t)json.number_value();
		}
		else if(json.is_string()) {
			if(json.string_value().empty()) {
				return std::nullopt;
			}
			return String(json.string_value()).maybeToArithmeticValue<size_t>();
		}
		else {
			return std::nullopt;
		}
	}

	Optional<bool> badlyFormattedBoolFromJson(const Json& json) {
		if(json.is_bool()) {
			return json.bool_value();
		}
		else if(json.is_number()) {
			if(json.number_value() == 0) {
				return false;
			}
			else if(json.number_value() > 0) {
				return true;
			}
			return std::nullopt;
		}
		else if(json.is_string()) {
			if(json.string_value().empty()) {
				return std::nullopt;
			}
			else if(json.string_value() == "0") {
				return false;
			}
			else if(json.string_value() == "1") {
				return true;
			}
			return std::nullopt;
		}
		else {
			return std::nullopt;
		}
	}

	Optional<double> badlyFormattedDoubleFromJson(const Json& json) {
		if(json.is_number()) {
			return json.number_value();
		}
		else if(json.is_string()) {
			if(json.string_value().empty()) {
				return std::nullopt;
			}
			return String(json.string_value()).maybeToArithmeticValue<double>();
		}
		else {
			return std::nullopt;
		}
	}

    String stringFromJson(const Json& json) {
        if(json.is_string()) {
            return json.string_value();
        }
        else if(json.is_number()) {
            if(floatIsIntegral(json.number_value())) {
                return std::to_string(numeric_cast<long long>(json.number_value()));
            } else {
                return std::to_string(json.number_value());
            }
        } else if(json.is_bool()) {
            return json.bool_value() ? "true" : "false";
        } else if(json.is_null()) {
            return String();
        } else {
            return json.dump();
        }
    }

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

	String nonNullStringPropFromNapiObject(Napi::Object obj, const char* propName) {
		Napi::String value = obj.Get(propName).As<Napi::String>();
		if(value.IsEmpty()) {
			throw std::runtime_error((std::string)"property "+propName+" is empty");
		} else if(value.IsUndefined()) {
			throw std::runtime_error((std::string)"property "+propName+" is undefined");
		} else if(value.IsNull()) {
			throw std::runtime_error((std::string)"property "+propName+" is null");
		} else if(!value.IsString()) {
			throw std::runtime_error((std::string)"property "+propName+" is not a string");
		}
		return value.Utf8Value();
	}

	String stringFromNapiValue(Napi::Value value) {
		if(value.IsEmpty() || value.IsNull() || value.IsUndefined()) {
			return String();
		}
		if(value.IsString()) {
			return value.As<Napi::String>().Utf8Value();
		}
		return value.ToString().Utf8Value();
	}

	Optional<String> optStringFromNapiValue(Napi::Value value) {
		if(value.IsEmpty() || value.IsNull() || value.IsUndefined()) {
			return std::nullopt;
		}
		if(value.IsString()) {
			return value.As<Napi::String>().Utf8Value();
		}
		return String(value.ToString().Utf8Value());
	}

	size_t sizeFromNapiValue(Napi::Value value) {
		if(value.IsEmpty() || value.IsNull() || value.IsUndefined()) {
			return 0;
		}
		return (size_t)value.As<Napi::Number>().Int64Value();
	}

	size_t nonNullSizePropFromNapiObject(Napi::Object obj, const char* propName) {
		Napi::Number value = obj.Get(propName).As<Napi::Number>();
		if(value.IsEmpty()) {
			throw std::runtime_error((std::string)"property "+propName+" is empty");
		} else if(value.IsUndefined()) {
			throw std::runtime_error((std::string)"property "+propName+" is undefined");
		} else if(value.IsNull()) {
			throw std::runtime_error((std::string)"property "+propName+" is null");
		} else if(!value.IsNumber()) {
			throw std::runtime_error((std::string)"property "+propName+" is not a number");
		}
		return (size_t)value.DoubleValue();
	}

	Optional<size_t> optSizeFromNapiValue(Napi::Value value) {
		if(value.IsEmpty() || value.IsNull() || value.IsUndefined()) {
			return std::nullopt;
		}
		return sizeFromNapiValue(value);
	}
	
	double doubleFromNapiValue(Napi::Value value) {
		if(value.IsEmpty() || value.IsNull() || value.IsUndefined()) {
			return 0;
		}
		return value.As<Napi::Number>().DoubleValue();
	}

	Optional<double> optDoubleFromNapiValue(Napi::Value value) {
		if(value.IsEmpty() || value.IsNull() || value.IsUndefined()) {
			return std::nullopt;
		}
		return doubleFromNapiValue(value);
	}

	bool boolFromNapiValue(Napi::Value value) {
		if(value.IsEmpty() || value.IsNull() || value.IsUndefined()) {
			return false;
		}
		return value.As<Napi::Boolean>().ToBoolean();
	}

	Optional<bool> optBoolFromNapiValue(Napi::Value value) {
		if(value.IsEmpty() || value.IsNull() || value.IsUndefined()) {
			return std::nullopt;
		}
		return boolFromNapiValue(value);
	}
}

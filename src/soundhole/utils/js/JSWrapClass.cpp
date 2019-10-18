//
//  JSWrapClass.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include <napi.h>
#include "JSWrapClass.hpp"
#include <soundhole/scripts/Scripts.hpp>
#include <embed/nodejs/NodeJS.hpp>

namespace sh {
	void JSWrapClass::queueJS(Function<void(napi_env)> work) {
		scripts::loadScriptsIfNeeded().then([=]() {
			embed::nodejs::queueMain([=](napi_env env) {
				this->initializeJS(env);
				work(env);
			});
		});
	}

	void JSWrapClass::queueJSDestruct(Function<void(napi_env)> work) {
		if(!embed::nodejs::isRunning()) {
			return;
		}
		scripts::loadScriptsIfNeeded().then([=]() {
			embed::nodejs::queueMain([=](napi_env env) {
				work(env);
			});
		});
	}

	Json JSWrapClass::jsonFromNAPIValue(napi_env env, napi_value value) {
		auto jsExports = scripts::getJSExports(env);
		auto resultJson = jsExports.Get("json_encode").As<Napi::Function>().Call({ value }).As<Napi::String>();
		std::string parseError;
		auto result = Json::parse(resultJson.Utf8Value(), parseError);
		if(!parseError.empty()) {
			throw std::runtime_error("Failed to convert napi_value to Json");
		}
		return result;
	}

	Promise<Json> JSWrapClass::jsonPromiseFromNAPIValue(napi_env env, napi_value value) {
		std::unique_ptr<Json> resultPtr;
		try {
			resultPtr = std::make_unique<Json>(jsonFromNAPIValue(env, value));
		} catch(...) {
			return Promise<Json>::reject(std::current_exception());
		}
		return Promise<Json>::resolve(std::move(*resultPtr.get()));
	}
}
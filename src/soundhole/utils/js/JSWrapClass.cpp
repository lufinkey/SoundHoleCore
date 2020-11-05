//
//  JSWrapClass.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "JSWrapClass.hpp"
#include <soundhole/scripts/Scripts.hpp>
#include <embed/nodejs/NodeJS.hpp>

namespace sh {
	JSWrapClass::JSWrapClass()
	: initializingJS(false), initializedJS(false) {
		
	}

	void JSWrapClass::initializeJSIfNeeded() {
		if(initializingJS) {
			return;
		}
		initializingJS = true;
		scripts::loadScriptsIfNeeded().then(nullptr, [=]() {
			embed::nodejs::queueMain([=](napi_env env) {
				if(!this->initializedJS) {
					this->initializedJS = true;
					this->initializingJS = false;
					this->initializeJS(env);
				}
				this->initializingJS = false;
			});
		}, [=](std::exception_ptr error) {
			this->initializingJS = false;
			std::rethrow_exception(error);
		});
	}

	void JSWrapClass::queueJS(Function<void(napi_env)> work) {
		scripts::loadScriptsIfNeeded().then(nullptr, [=]() {
			embed::nodejs::queueMain([=](napi_env env) {
				if(!this->initializedJS) {
					this->initializedJS = true;
					this->initializingJS = false;
					this->initializeJS(env);
				}
				this->initializingJS = false;
				work(env);
			});
		});
	}

	void JSWrapClass::queueJSDestruct(Function<void(napi_env)> work) {
		if(!embed::nodejs::isRunning()) {
			return;
		}
		scripts::loadScriptsIfNeeded().then(nullptr, [=]() {
			embed::nodejs::queueMain([=](napi_env env) {
				work(env);
			});
		});
	}
}

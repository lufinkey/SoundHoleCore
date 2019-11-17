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
}

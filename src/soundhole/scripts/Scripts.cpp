//
//  Scripts.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "Scripts.hpp"
#include <mutex>
#include <embed/nodejs/NodeJS.hpp>
#include <embed/nodejs/NAPI_Macros.hpp>
#include <napi.h>
#include <nodejs/node_api.h>
#include "soundhole_js_bundle.h"

namespace sh::scripts {
	bool scriptsLoaded = false;
	std::unique_ptr<Promise<void>> scriptsLoadPromise;
	std::mutex scriptsLoadPromiseMutex;
	
	String moduleName = "[eval]/soundhole";
	Napi::Reference<Napi::Object> moduleExports;
	
	#ifndef NAPI_CALL_OR_THROW
		#define NAPI_CALL_OR_THROW(env, error, the_call) NAPI_CALL_BASE(env, the_call, throw std::runtime_error(error))
	#endif
	
	Promise<void> loadScriptsIfNeeded() {
		std::unique_lock<std::mutex> lock(scriptsLoadPromiseMutex);
		if(scriptsLoaded) {
			return Promise<void>::resolve();
		}
		if(scriptsLoadPromise) {
			return *scriptsLoadPromise.get();
		}
		scriptsLoadPromise = std::make_unique<Promise<void>>([](auto resolve, auto reject) {
			try {
				if(!embed::nodejs::isRunning()) {
					embed::nodejs::start();
				}
				embed::nodejs::queueMain([=](napi_env env) {
					try {
						napi_value moduleVal = embed::nodejs::loadModuleFromMemory(env, moduleName, (const char*)soundhole_js_bundle_js, (size_t)soundhole_js_bundle_js_len);
						Napi::Object module(env, moduleVal);
						Napi::Object exports = module.Get("exports").As<Napi::Object>();
						napi_ref exportsRef = nullptr;
						auto exportsRefError = "failed to create reference for soundhole exports";
						NAPI_CALL_OR_THROW(env, exportsRefError, napi_create_reference(env, exports, 1, &exportsRef));
						moduleExports = Napi::Reference<Napi::Object>(env, exportsRef);
					} catch(Napi::Error& error) {
						std::unique_lock<std::mutex> lock(scriptsLoadPromiseMutex);
						scriptsLoadPromise.reset();
						lock.unlock();
						reject(std::runtime_error(error.what()));
						return;
					} catch(...) {
						std::unique_lock<std::mutex> lock(scriptsLoadPromiseMutex);
						scriptsLoadPromise.reset();
						lock.unlock();
						reject(std::current_exception());
						return;
					}
					std::unique_lock<std::mutex> lock(scriptsLoadPromiseMutex);
					scriptsLoaded = true;
					scriptsLoadPromise.reset();
					lock.unlock();
					resolve();
				});
			} catch(...) {
				reject(std::current_exception());
			}
		});
		return *scriptsLoadPromise.get();
	}
	
	napi_ref getJSExportsRef() {
		return moduleExports;
	}
}
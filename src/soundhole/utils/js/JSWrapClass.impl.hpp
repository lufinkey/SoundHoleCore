//
//  JSWrapClass.impl.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/27/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "JSUtils.hpp"

namespace sh {
	#ifdef NODE_API_MODULE
	template<typename Result>
	Promise<Result> JSWrapClass::performAsyncFunc(napi_ref jsRef, String funcName, Function<std::vector<napi_value>(napi_env)> createArgs, Function<Result(napi_env,Napi::Value)> mapper, PerformAsyncFuncOptions options) {
		return Promise<Result>([=](auto resolve, auto reject) {
			queueJS([=](napi_env env) {
				if(options.beforeFuncCall != nullptr) {
					options.beforeFuncCall(env);
				}
				// get api object and make call
				auto jsApi = jsutils::jsValue<Napi::Object>(env, jsRef);
				if(jsApi.IsEmpty()) {
					reject(std::logic_error("js object not initialized"));
					return;
				}
				auto promise = jsApi.Get(funcName).As<Napi::Function>().Call(jsApi, createArgs(env)).As<Napi::Object>();
				promise.Get("then").As<Napi::Function>().Call(promise, {
					// then
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						napi_env env = info.Env();
						if(options.afterFuncFinish != nullptr) {
							options.afterFuncFinish(env);
						}
						// decode result
						auto value = info[0];
						if constexpr(std::is_same<Result,void>::value) {
							if(mapper != nullptr) {
								try {
									mapper(env,value);
								} catch(Napi::Error& error) {
									reject(std::runtime_error(error.what()));
									return;
								} catch(...) {
									reject(std::current_exception());
									return;
								}
							}
							resolve();
						} else {
							Optional<Result> result;
							try {
								result = mapper(env,value);
							} catch(Napi::Error& error) {
								reject(std::runtime_error(error.what()));
								return;
							} catch(...) {
								reject(std::current_exception());
								return;
							}
							resolve(std::move(result.value()));
						}
					}),
					// catch
					Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
						napi_env env = info.Env();
						if(options.afterFuncFinish != nullptr) {
							options.afterFuncFinish(env);
						}
						// handle error
						auto errorMessage = info[0].As<Napi::Object>().Get("message").ToString().Utf8Value();
						reject(std::runtime_error(errorMessage));
					})
				});
			});
		});
	}
	#endif
}

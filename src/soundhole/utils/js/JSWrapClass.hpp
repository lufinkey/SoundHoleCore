//
//  JSWrapClass.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "JSUtils.hpp"

namespace sh {
	class JSWrapClass {
	protected:
		JSWrapClass();
		
		virtual void initializeJS(napi_env) = 0;
		void initializeJSIfNeeded();
		void queueJS(Function<void(napi_env)> work);
		void queueJSDestruct(Function<void(napi_env)> work);
		
		#ifdef NODE_API_MODULE
		struct PerformAsyncFuncOptions {
			Function<void(napi_env)> beforeFuncCall;
			Function<void(napi_env)> afterFuncFinish;
		};
		template<typename Result>
		Promise<Result> performAsyncFunc(
			Function<Napi::Value(napi_env)> jsGetter,
			String funcName,
			Function<std::vector<napi_value>(napi_env)> createArgs,
			Function<Result(napi_env,Napi::Value)> mapper,
			PerformAsyncFuncOptions options = PerformAsyncFuncOptions());
		#endif
		
	private:
		bool initializingJS;
		bool initializedJS;
	};
}

//
//  JSError.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 7/16/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include <napi.h>
#include <soundhole/utils/js/JSError.hpp>
#include <soundhole/scripts/Scripts.hpp>
#include <soundhole/utils/js/JSUtils.hpp>
#include <embed/nodejs/NodeJS.hpp>

namespace sh {
	JSError::JSError(napi_env env, napi_value val): jsRef(nullptr) {
		auto obj = Napi::Object(env, val);
		// save js properties
		code = obj.Get("code").ToString().Utf8Value();
		stack = obj.Get("stack").ToString().Utf8Value();
		message = obj.Get("message").ToString().Utf8Value();
		// store js reference
		auto ref = Napi::Persistent(Napi::Object(env, val));
		ref.SuppressDestruct();
		jsRef = ref;
	}

	JSError::JSError(JSError&& err)
	: jsRef(err.jsRef),
	code(err.code),
	stack(err.stack),
	message(err.message) {
		err.jsRef = nullptr;
	}

	JSError::JSError(const JSError& err)
	: jsRef(err.jsRef),
	code(err.code),
	stack(err.stack),
	message(err.message) {
		// queue call to "ref"
		auto ref = this->jsRef;
		if(ref != nullptr && embed::nodejs::isRunning()) {
			embed::nodejs::queueMain([=](napi_env env) {
				auto napiRef = Napi::ObjectReference(env, ref);
				napiRef.Ref();
				napiRef.SuppressDestruct();
			});
		}
	}

	JSError::~JSError() {
		// queue call to destroy js reference
		auto ref = this->jsRef;
		if(ref != nullptr && embed::nodejs::isRunning()) {
			embed::nodejs::queueMain([=](napi_env env) {
				auto napiRef = Napi::ObjectReference(env, ref);
				if(napiRef.Unref() == 0) {
					napiRef.Reset();
				} else {
					napiRef.SuppressDestruct();
				}
			});
		}
	}

	napi_ref JSError::getJSRef() const {
		return jsRef;
	}

	napi_value JSError::getJSValue(napi_env env) const {
		return jsutils::jsValue<Napi::Object>(env, jsRef);
	}

	String JSError::getMessage() const {
		return message;
	}

	String JSError::toString() const {
		return String::join({
			code,": ",message,"\n",stack
		});
	}

	Any JSError::getDetail(const String& key) const {
		if(key == "code") {
			return code;
		}
		else if(key == "stack") {
			return stack;
		}
		return BasicError<>::getDetail(key);
	}

	const String& JSError::getJSStack() const {
		return stack;
	}
}

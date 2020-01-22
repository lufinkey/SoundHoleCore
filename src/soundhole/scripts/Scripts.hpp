//
//  Scripts.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

struct napi_ref__;
typedef struct napi_ref__* napi_ref;

namespace sh::scripts {
	Promise<void> loadScriptsIfNeeded();
	
	napi_ref getJSExportsRef();
	#ifdef NODE_API_MODULE
	inline Napi::Object getJSExports(napi_env env) {
		auto exportsRef = scripts::getJSExportsRef();
		if(exportsRef == nullptr) {
			return Napi::Object();
		}
		auto exportsNapiRef = Napi::ObjectReference(env, exportsRef);
		exportsNapiRef.SuppressDestruct();
		return exportsNapiRef.Value().template As<Napi::Object>();
	}
	#endif
}

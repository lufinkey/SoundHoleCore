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
	Napi::Object getJSExports(napi_env env);
	Napi::Value parseJsonToNapi(napi_env env, const std::string& json);
	String napiToJson(napi_env, napi_value);
	String napiToPrettyJson(napi_env, napi_value);
	#endif
}

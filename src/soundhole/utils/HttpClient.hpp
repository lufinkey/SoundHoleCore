//
//  HttpClient.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/19/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <map>
#include <cxxurl/url.hpp>

namespace sh::utils {
	enum class HttpMethod {
		GET,
		HEAD,
		POST,
		PUT,
		DELETE,
		CONNECT,
		OPTIONS,
		TRACE,
		PATCH
	};
	
	String HttpMethod_toString(HttpMethod);
	
	struct HttpRequest {
		Url url;
		HttpMethod method;
		std::map<String,String> headers;
		String data;
	};
	
	struct HttpResponse {
		int statusCode;
		String statusMessage;
		std::map<String,String> headers;
		String data;
	};
	
	Promise<std::shared_ptr<HttpResponse>> performHttpRequest(HttpRequest request);
}

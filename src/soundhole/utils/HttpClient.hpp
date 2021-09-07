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

	struct HttpHeaders {
		using ValueVariant = Variant<ArrayList<String>,String>;
		
		HttpHeaders();
		HttpHeaders(Map<String,ValueVariant> map);
		HttpHeaders(const Map<String,String>& map);
		
		Map<String,ValueVariant> map;
		
		void add(String key, String value);
		void set(String key, String value);
		void set(String key, ArrayList<String> values);
		ArrayList<String> get(String key) const;
	};
	
	struct HttpRequest {
		Url url;
		HttpMethod method;
		HttpHeaders headers;
		String data;
	};
	
	struct HttpResponse {
		int statusCode;
		String statusMessage;
		HttpHeaders headers;
		String data;
	};
	using SharedHttpResponse = std::shared_ptr<HttpResponse>;
	
	Promise<SharedHttpResponse> performHttpRequest(HttpRequest request);

	String encodeURLComponent(String);
	String decodeURLComponent(String);
	String makeQueryString(std::map<String,String> params);
	Map<String,String> parseQueryString(const String& queryString);
	std::map<String,String> parseURLQueryParams(String url);

	bool checkURLMatch(const String& baseURL, const String& url);
}

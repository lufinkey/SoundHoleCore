 //
//  HttpClient.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/19/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include <napi.h>
#include "HttpClient.hpp"
#include <stdexcept>
#include <embed/nodejs/NodeJS.hpp>
#include <soundhole/scripts/Scripts.hpp>

namespace sh::utils {
	String HttpMethod_toString(HttpMethod method) {
		switch(method) {
			case HttpMethod::GET:
				return "GET";
			case HttpMethod::HEAD:
				return "HEAD";
			case HttpMethod::POST:
				return "POST";
			case HttpMethod::PUT:
				return "PUT";
			case HttpMethod::DELETE:
				return "DELETE";
			case HttpMethod::CONNECT:
				return "CONNECT";
			case HttpMethod::OPTIONS:
				return "OPTIONS";
			case HttpMethod::TRACE:
				return "TRACE";
			case HttpMethod::PATCH:
				return "PATCH";
			default:
				throw std::invalid_argument("Invalid HttpMethod value");
		}
	}



	HttpHeaders::HttpHeaders() {
		//
	}

	HttpHeaders::HttpHeaders(Map<String,ValueVariant> map)
	: map(map) {
		//
	}

	HttpHeaders::HttpHeaders(const Map<String,String>& map)
	: map(map.mapValues([](auto& key, auto& val) { return ValueVariant(val); })) {
		//
	}

	void HttpHeaders::add(String key, String value) {
		auto it = map.find(key);
		if(it == map.end()) {
			map.put(key, ArrayList<String>{ value });
			return;
		}
		auto array = it->second.visit([](auto& val) {
			if constexpr(std::is_same_v<String,std::remove_cvref_t<decltype(val)>>) {
				return ArrayList<String>{ val };
			} else {
				return val;
			}
		});
		array.pushBack(value);
		it->second = ValueVariant(array);
	}

	void HttpHeaders::set(String key, String value) {
		map.put(key, ValueVariant(value));
	}

	void HttpHeaders::set(String key, ArrayList<String> values) {
		map.put(key, ValueVariant(values));
	}

	ArrayList<String> HttpHeaders::get(String key) const {
		return map.get(key, ValueVariant(ArrayList<String>{})).visit([](auto& val) {
			if constexpr(std::is_same_v<String,std::remove_cvref_t<decltype(val)>>) {
				return ArrayList<String>{ val };
			} else {
				return val;
			}
		});
	}
	


	#if !defined(TARGETPLATFORM_IOS) && !defined(TARGETPLATFORM_MAC)
	Promise<SharedHttpResponse> performHttpRequest(HttpRequest request) {
		return scripts::loadScriptsIfNeeded().then([=]() -> Promise<SharedHttpResponse> {
			return Promise<SharedHttpResponse>([&](auto resolve, auto reject) {
				try {
					embed::nodejs::queueMain([=](napi_env env) {
						try {
							Napi::ObjectReference exportsRef(env, scripts::getJSExportsRef());
							exportsRef.SuppressDestruct();
							Napi::Object exports = exportsRef.Value();
							Napi::Function performRequest = exports.Get("performHttpRequest").As<Napi::Function>();
							auto jsRequest = Napi::Object::New(env);
							jsRequest.Set("url", Napi::String::New(env, (const std::string&)request.url.toString()));
							jsRequest.Set("method", Napi::String::New(env, (const std::string&)HttpMethod_toString(request.method)));
							if(request.headers.size() > 0) {
								auto headers = Napi::Object::New(env);
								for(auto& header : request.headers.map) {
									if(header.second.is<String>()) {
										headers.Set((const std::string&)header.first, Napi::String::New(env, (const std::string&)header.second.get<String>()));
									} else {
										auto& arrayVal = header.second.get<ArrayList<String>>();
										if(!arrayVal.empty()) {
											headers.Set((const std::string&)header.first, arrayVal.toNapiArray(env, [](auto env, auto& val) {
												return Napi::String::New(env, (const std::string&)val);
											}));
										}
									}
								}
								jsRequest.Set("headers", headers);
							}
							if(request.data.size() > 0) {
								jsRequest.Set("data", Napi::Buffer<char>::Copy(env, request.data.data(), request.data.size()));
							}
							performRequest.Call({ jsRequest, Napi::Function::New(env, [=](const Napi::CallbackInfo& info) {
								Napi::Object errorObj = info[0].As<Napi::Object>();
								Napi::Object responseObj = info[1].As<Napi::Object>();
								if(!responseObj.IsNull()) {
									auto response = std::make_shared<HttpResponse>();
									response->statusCode = (int)responseObj.Get("statusCode").ToNumber();
									response->statusMessage = (std::string)responseObj.Get("statusMessage").As<Napi::String>();
									auto headersObj = responseObj.Get("headers").As<Napi::Object>();
									auto headerNames = headersObj.GetPropertyNames();
									for(uint32_t i=0; i<headerNames.Length(); i++) {
										auto headerName = headerNames.Get(i).As<Napi::String>();
										auto headerVal = headersObj.Get(headerName);
										if(headerVal.IsArray()) {
											auto headerValArray = headerVal.As<Napi::Array>();
											ArrayList<String> values;
											size_t valuesSize = headerValArray.Length();
											values.reserve(valuesSize);
											for(size_t i=0; i<valuesSize; i++) {
												auto headerValItem = headerValArray.Get(i);
												values.push((const std::string&)headerValItem.As<Napi::String>());
											}
											response->headers.set((const std::string&)headerName, values);
										}
										else {
											response->headers.set((const std::string&)headerName, headerVal.As<Napi::String>());
										}
									}
									auto dataBuffer = responseObj.Get("data").As<Napi::Uint8Array>().ArrayBuffer();
									response->data = String((const char*)dataBuffer.Data(), (size_t)dataBuffer.ByteLength());
									resolve(response);
								} else {
									reject(std::runtime_error((std::string)errorObj.ToString()));
								}
							}) });
						} catch(Napi::Error& error) {
							reject(std::runtime_error(error.what()));
							error.Unref();
						} catch(...) {
							reject(std::current_exception());
						}
					});
				} catch(...) {
					reject(std::current_exception());
				}
			});
		});
	}
	#endif



	Map<String,String> parseURLQueryParams(String urlString) {
		auto params = Map<String,String>();
		auto queryItems = URL(urlString).queryItems();
		for(auto& item : queryItems) {
			params.insert_or_assign(item.key, item.value.valueOr((String())));
		}
		return params;
	}

	bool checkURLMatch(const String& baseURLString, const String& urlString) {
		if(urlString.empty() || baseURLString.empty()) {
			return false;
		}
		if(!urlString.startsWith(baseURLString)) {
			return false;
		}
		auto baseURL = URL(baseURLString);
		auto url = URL(urlString);
		auto path = baseURL.path();
		if(path == "/") {
			path = "";
		}
		auto cmpPath = url.path();
		if(cmpPath == "/") {
			cmpPath = "";
		}
		if(path != cmpPath) {
			return false;
		}
		return true;
	}
}

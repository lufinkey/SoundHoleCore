//
//  HttpClient.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/19/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "HttpClient.hpp"
#include <stdexcept>
#include <napi.h>
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
	
	Promise<std::shared_ptr<HttpResponse>> performHttpRequest(HttpRequest request) {
		return scripts::loadScriptsIfNeeded().then([=]() -> Promise<std::shared_ptr<HttpResponse>> {
			return Promise<std::shared_ptr<HttpResponse>>([&](auto resolve, auto reject) {
				try {
					embed::nodejs::queueMain([=](napi_env env) {
						try {
							Napi::Reference<Napi::Object> exportsRef(env, scripts::getJSExportsRef());
							Napi::Object exports = exportsRef.Value();
							Napi::Function performRequest = exports.Get("performHttpRequest").As<Napi::Function>();
							auto jsRequest = Napi::Object::New(env);
							jsRequest.Set("url", Napi::String::New(env, request.url.str()));
							jsRequest.Set("method", Napi::String::New(env, (const std::string&)HttpMethod_toString(request.method)));
							if(request.headers.size() > 0) {
								auto headers = Napi::Object::New(env);
								for(auto& header : request.headers) {
									headers.Set((const std::string&)header.first, Napi::String::New(env, (const std::string&)header.second));
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
										auto headerName = headerNames.Get(i);
										response->headers[(std::string)headerName.ToString()] = (std::string)headersObj.Get(headerName).ToString();
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
}

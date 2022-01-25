//
//  HttpClient.m
//  SoundHoleCore
//
//  Created by Luis Finke on 11/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "HttpClient.hpp"

#ifdef __OBJC__
#import <Foundation/Foundation.h>

namespace sh::utils {
	#if defined(TARGETPLATFORM_IOS) || defined(TARGETPLATFORM_MAC)
	Promise<SharedHttpResponse> performHttpRequest(HttpRequest req) {
		NSURL* url = [NSURL URLWithString:[NSString stringWithUTF8String:req.url.toString().c_str()]];
		if(url == nil) {
			return Promise<SharedHttpResponse>::reject(std::logic_error("Invalid URL"));
		}
		NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url];
		request.HTTPMethod = HttpMethod_toString(req.method).toNSString();
		for(auto& header : req.headers.map) {
			if(header.second.is<String>()) {
				[request setValue:header.second.get<String>().toNSString() forHTTPHeaderField:header.first.toNSString()];
			} else {
				for(auto val : header.second.get<ArrayList<String>>()) {
					[request addValue:val.toNSString() forHTTPHeaderField:header.first.toNSString()];
				}
			}
		}
		if(!req.data.empty()) {
			request.HTTPBody = [NSData dataWithBytes:(const void*)req.data.c_str() length:(NSUInteger)req.data.length()];
		}
		return Promise<SharedHttpResponse>([=](auto resolve, auto reject) {
			[[NSURLSession.sharedSession dataTaskWithRequest:request completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
				if(error != nil) {
					reject(std::runtime_error(String(error.localizedDescription)));
					return;
				}
				if(![response isKindOfClass:NSHTTPURLResponse.class]) {
					reject(std::runtime_error("Unknown response type"));
					return;
				}
				NSHTTPURLResponse* httpResponse = (NSHTTPURLResponse*)response;
				auto res = std::make_shared<HttpResponse>();
				res->statusCode = (int)httpResponse.statusCode;
				res->statusMessage = String([NSHTTPURLResponse localizedStringForStatusCode:httpResponse.statusCode]);
				NSDictionary* allHeaders = httpResponse.allHeaderFields;
				for(NSString* headerName in allHeaders.allKeys) {
					id headerValue = allHeaders[headerName];
					if([headerValue isKindOfClass:NSString.class]) {
						res->headers.set(String(headerName), String((NSString*)headerValue));
					} else if([headerValue isKindOfClass:NSArray.class]) {
						NSArray* headerValues = headerValue;
						for(id headerVal in headerValues) {
							if([headerVal isKindOfClass:NSString.class]) {
								res->headers.add(String(headerName), String((NSString*)headerVal));
							} else if([headerVal isKindOfClass:NSNumber.class]) {
								res->headers.add(String(headerName), String(((NSNumber*)headerVal).stringValue));
							}
						}
					}
				}
				if(data != nil) {
					res->data = String((const char*)data.bytes, (size_t)data.length);
				}
				resolve(res);
			}] resume];
		});
	}
	#endif
}

#endif

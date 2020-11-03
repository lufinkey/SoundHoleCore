//
//  SHObjcUtils.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 11/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#import "SHObjcUtils.h"

@implementation SHObjcUtils

+(NSDictionary*)decodeQueryString:(NSString*)queryString {
	NSArray<NSString*>* parts = [queryString componentsSeparatedByString:@"&"];
	NSMutableDictionary* params = [NSMutableDictionary dictionary];
	for(NSString* part in parts) {
		NSString* escapedPart = [part stringByReplacingOccurrencesOfString:@"+" withString:@"%20"];
		NSArray<NSString*>* expressionParts = [escapedPart componentsSeparatedByString:@"="];
		if(expressionParts.count != 2) {
			continue;
		}
		NSString* key = expressionParts[0].stringByRemovingPercentEncoding;
		NSString* value = expressionParts[1].stringByRemovingPercentEncoding;
		params[key] = value;
	}
	return params;
}

+(NSDictionary*)parseOAuthQueryParams:(NSURL*)url {
	if(url == nil) {
		return [NSDictionary dictionary];
	}
	NSDictionary* queryParams = [self decodeQueryString:url.query];
	if(queryParams != nil && queryParams.count > 0) {
		return queryParams;
	}
	NSDictionary* fragmentParams = [self decodeQueryString:url.fragment];
	if(fragmentParams != nil && fragmentParams.count > 0) {
		return fragmentParams;
	}
	return [NSDictionary dictionary];
}

+(BOOL)checkIfURL:(NSURL*)url matchesRedirectURL:(NSURL*)redirectURL {
	if(redirectURL == nil) {
		return NO;
	}
	if(![url.absoluteString hasPrefix:redirectURL.absoluteString]) {
		return NO;
	}
	NSString* path = redirectURL.path;
	if(path == nil || [path isEqualToString:@"/"]) {
		path = @"";
	}
	NSString* cmpPath = url.path;
	if(cmpPath == nil || [cmpPath isEqualToString:@"/"]) {
		cmpPath = @"";
	}
	if(![path isEqualToString:cmpPath]) {
		return NO;
	}
	return YES;
}

+(void)runOnMain:(void(^)())executor {
	if([NSThread isMainThread]) {
		executor();
	} else {
		dispatch_async(dispatch_get_main_queue(), ^{
			executor();
		});
	}
}

@end

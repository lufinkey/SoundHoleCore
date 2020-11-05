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


+(NSString*)httpDateString:(NSDate*)date {
	NSDateFormatter* formatter = [[NSDateFormatter alloc] init];
	formatter.locale = [NSLocale localeWithLocaleIdentifier:@"en_US_POSIX"];
	formatter.timeZone = [NSTimeZone timeZoneWithAbbreviation:@"GMT"];
	formatter.dateFormat = @"EEE, dd MMM yyyy HH:mm:ss O";
	return [formatter stringFromDate:date];
}


+(NSString*)stringFromCookie:(NSHTTPCookie*)cookie {
	NSMutableString* cookieString = [NSMutableString stringWithFormat:@"%@=%@; Domain=%@; Path=%@", cookie.name, cookie.value, cookie.domain, cookie.path];
	if(cookie.expiresDate != nil) {
		[cookieString appendFormat:@"; Expires=%@", [self httpDateString:cookie.expiresDate]];
	}
	if(cookie.version != 1) {
		[cookieString appendFormat:@"; Version=%i", (int)cookie.version];
	}
	if(cookie.isSecure) {
		[cookieString appendFormat:@"; Secure"];
	}
	if(cookie.HTTPOnly) {
		[cookieString appendFormat:@"; HttpOnly"];
	}
#ifdef TARGETPLATFORM_IOS
	if (@available(iOS 13.0, *)) {
		if(cookie.sameSitePolicy != nil) {
			[cookieString appendFormat:@"; SameSite=%@", cookie.sameSitePolicy];
		}
	}
#elif TARGETPLATFORM_MAC
	if(@available(macOS 10.15, *)) {
		if(cookie.sameSitePolicy != nil) {
			[cookieString appendFormat:@"; SameSite=%@", cookie.sameSitePolicy];
		}
	}
#endif
	return cookieString;
}

+(NSArray<NSString*>*)stringsFromCookies:(NSArray<NSHTTPCookie*>*)cookies {
	NSMutableArray<NSString*>* cookieStrings = [NSMutableArray array];
	for(NSHTTPCookie* cookie in cookies) {
		[cookieStrings addObject:[self stringFromCookie:cookie]];
	}
	return cookieStrings;
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

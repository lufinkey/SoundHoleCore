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


sh::Json sh::utils::jsonFromNSObject(NSObject* object) {
	if(object == nil || [object isKindOfClass:NSNull.class]) {
		return nullptr;
	}
	else if([object isKindOfClass:NSString.class]) {
		NSString* string = (NSString*)object;
		return string.UTF8String;
	}
	else if([object isKindOfClass:NSNumber.class]) {
		NSNumber* num = (NSNumber*)object;
		if(strcmp(num.objCType, @encode(bool)) || strcmp(num.objCType, @encode(BOOL))) {
			return num.boolValue;
		}
		else {
			return num.doubleValue;
		}
	}
	else if([object isKindOfClass:NSData.class]) {
		NSData* data = (NSData*)object;
		return [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding].UTF8String;
	}
	else if([object isKindOfClass:NSDate.class]) {
		NSDate* date = (NSDate*)object;
		NSDateFormatter* dateFormatter = [[NSDateFormatter alloc] init];
		dateFormatter.locale = [NSLocale localeWithLocaleIdentifier:@"en_US_POSIX"];
		dateFormatter.dateFormat = @"yyyy-MM-dd'T'HH:mm:ssZZZZZ";
		dateFormatter.calendar = [NSCalendar calendarWithIdentifier:NSCalendarIdentifierGregorian];
		return [dateFormatter stringFromDate:date].UTF8String;
	}
	else if([object isKindOfClass:NSURL.class]) {
		NSURL* url = (NSURL*)object;
		return url.absoluteString.UTF8String;
	}
	else if([object isKindOfClass:NSDictionary.class]) {
		auto jsonObj = sh::Json::object{};
		NSDictionary* dict = (NSDictionary*)object;
		for(NSString* key in dict) {
			jsonObj[key.UTF8String] = jsonFromNSObject(dict[key]);
		}
		return jsonObj;
	}
	else if([object isKindOfClass:NSArray.class]) {
		auto jsonArray = sh::Json::array{};
		NSArray* arr = (NSArray*)object;
		jsonArray.reserve((size_t)arr.count);
		for(NSObject* obj in arr) {
			jsonArray.push_back(jsonFromNSObject(obj));
		}
		return jsonArray;
	}
	else {
		throw std::invalid_argument((std::string)"Invalid class type "+NSStringFromClass(object.class).UTF8String);
	}
}


NSObject* sh::utils::nsObjectFromJson(Json json) {
	if(json.is_null()) {
		return nil;
	}
	else if(json.is_bool()) {
		return [NSNumber numberWithBool:json.bool_value()];
	}
	else if(json.is_number()) {
		return [NSNumber numberWithDouble:json.number_value()];
	}
	else if(json.is_object()) {
		NSMutableDictionary* dict = [NSMutableDictionary dictionary];
		for(auto& pair : json.object_items()) {
			NSString* key = [NSString stringWithUTF8String:pair.first.c_str()];
			NSObject* obj = nsObjectFromJson(pair.second);
			if(obj == nil) {
				obj = [NSNull null];
			}
			dict[key] = obj;
		}
		return dict;
	}
	else if(json.is_array()) {
		NSMutableArray* arr = [NSMutableArray array];
		for(auto& item : json.array_items()) {
			NSObject* obj = nsObjectFromJson(item);
			if(obj == nil) {
				obj = [NSNull null];
			}
			[arr addObject:obj];
		}
		return arr;
	}
	else {
		throw std::invalid_argument((std::string)"Invalid json type "+std::to_string(json.type()));
	}
}

@end

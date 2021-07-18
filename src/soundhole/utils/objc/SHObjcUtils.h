//
//  SHObjcUtils.h
//  SoundHoleCore
//
//  Created by Luis Finke on 11/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include <soundhole/common.hpp>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface SHObjcUtils : NSObject

+(NSDictionary*)decodeQueryString:(NSString*)queryString;
+(NSDictionary*)parseOAuthQueryParams:(NSURL*)url;
+(BOOL)checkIfURL:(NSURL*)url matchesRedirectURL:(NSURL*)redirectURL;

+(NSString*)httpDateString:(NSDate*)date;
+(NSString*)stringFromCookie:(NSHTTPCookie*)cookie;
+(NSArray<NSString*>*)stringsFromCookies:(NSArray<NSHTTPCookie*>*)cookies;

+(void)runOnMain:(void(^)(void))executor;

@end

namespace sh::utils {
	Json jsonFromNSObject(NSObject*);
	NSObject* nsObjectFromJson(Json);
}

NS_ASSUME_NONNULL_END

//
//  SHObjcUtils.h
//  SoundHoleCore
//
//  Created by Luis Finke on 11/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface SHObjcUtils : NSObject

+(NSDictionary*)decodeQueryString:(NSString*)queryString;
+(NSDictionary*)parseOAuthQueryParams:(NSURL*)url;
+(BOOL)checkIfURL:(NSURL*)url matchesRedirectURL:(NSURL*)redirectURL;
+(void)runOnMain:(void(^)(void))executor;

@end

NS_ASSUME_NONNULL_END

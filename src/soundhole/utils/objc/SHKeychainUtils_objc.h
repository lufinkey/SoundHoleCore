//
//  SHKeychainUtils_objc.h
//  SoundHoleCore
//
//  Created by Luis Finke on 10/27/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#ifdef __OBJC__
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface SHKeychainUtils: NSObject

+(nullable NSData*)genericPasswordDataForAccountKey:(NSString*)accountKey;
+(BOOL)deleteGenericPasswordDataForAccountKey:(NSString*)accountKey;
+(BOOL)setGenericPasswordData:(NSData*)data forAccountKey:(NSString*)accountKey;

@end

NS_ASSUME_NONNULL_END

#endif

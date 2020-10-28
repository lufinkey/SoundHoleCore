//
//  KeychainUtils.m
//  SoundHoleCore
//
//  Created by Luis Finke on 10/27/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#ifdef __OBJC__
#import "SHKeychainUtils_ObjC.h"
#import <Security/Security.h>

@implementation SHKeychainUtils

+(NSData*)genericPasswordDataForAccountKey:(NSString*)accountKey {
	NSData* accountKeyData = [accountKey dataUsingEncoding:NSUTF8StringEncoding];
	NSDictionary* query = @{
		(NSString*)kSecMatchLimit: (NSString*)kSecMatchLimitOne,
		(NSString*)kSecClass: (NSString*)kSecClassGenericPassword,
		(NSString*)kSecAttrAccount: accountKeyData,
		(NSString*)kSecReturnAttributes: @(YES),
		(NSString*)kSecReturnData: @(YES)
	};
	CFTypeRef result = nil;
	OSStatus status = SecItemCopyMatching((CFDictionaryRef)query, &result);
	if(status != noErr) {
		return nil;
	}
	NSDictionary* resultDict = (__bridge NSDictionary*)result;
	return resultDict[(NSString*)kSecValueData];
}

+(BOOL)deleteGenericPasswordDataForAccountKey:(NSString*)accountKey {
	NSData* accountKeyData = [accountKey dataUsingEncoding:NSUTF8StringEncoding];
	NSDictionary* query = @{
		(NSString*)kSecClass: (NSString*)kSecClassGenericPassword,
		(NSString*)kSecAttrAccount: accountKeyData
	};
	OSStatus deleteStatus = SecItemDelete((CFDictionaryRef)query);
	return (deleteStatus == noErr || deleteStatus == errSecItemNotFound);
}

+(BOOL)setGenericPasswordData:(NSData*)data forAccountKey:(NSString*)accountKey {
	NSData* accountKeyData = [accountKey dataUsingEncoding:NSUTF8StringEncoding];
	NSMutableDictionary* query = [NSMutableDictionary dictionaryWithDictionary:@{
		(NSString*)kSecClass: (NSString*)kSecClassGenericPassword,
		(NSString*)kSecAttrAccount: accountKeyData
	}];
	OSStatus deleteStatus = SecItemDelete((CFDictionaryRef)query);
	if(deleteStatus != noErr && deleteStatus != errSecItemNotFound) {
		return NO;
	}
	query[(NSString*)kSecValueData] = data;
	OSStatus addStatus = SecItemAdd((CFDictionaryRef)query, nil);
	return (addStatus != noErr);
}

@end

#endif

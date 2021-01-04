//
//  SoundHoleMediaProvider_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 1/3/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "SoundHoleMediaProvider.hpp"

#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <soundhole/utils/ios/SHiOSUtils.h>

namespace sh {
	void SoundHoleMediaProvider::loadUserPrefs() {
		if(sessionPersistKey.empty()) {
			return;
		}
		NSDictionary* prefs = [NSUserDefaults.standardUserDefaults objectForKey:sessionPersistKey.toNSString()];
		if(prefs == nil || ![prefs isKindOfClass:NSDictionary.class]) {
			return;
		}
		NSString* primaryStorageProvider = prefs[@"primaryStorageProvider"];
		primaryStorageProviderName = (primaryStorageProvider != nil) ? String(primaryStorageProvider) : String();
	}
	
	void SoundHoleMediaProvider::saveUserPrefs() {
		if(sessionPersistKey.empty()) {
			return;
		}
		NSMutableDictionary* prefs = [NSMutableDictionary dictionary];
		if(!primaryStorageProviderName.empty()) {
			prefs[@"primaryStorageProvider"] = primaryStorageProviderName.toNSString();
		}
		[NSUserDefaults.standardUserDefaults setObject:prefs forKey:sessionPersistKey.toNSString()];
	}
	
	Promise<bool> SoundHoleMediaProvider::login() {
		return Promise<bool>([=](auto resolve, auto reject) {
			if(storageProviders.size() == 0) {
				reject(std::runtime_error("SoundHoleMediaProvider has no storage providers"));
				return;
			} else if(storageProviders.size() == 1) {
				auto storageProvider = storageProviders.front();
				storageProvider->login().then([=](bool loggedIn) {
					if(loggedIn) {
						setPrimaryStorageProvider(storageProvider);
					}
					resolve(loggedIn);
				}).except(reject);
				return;
			}
			UIViewController* topViewController = [SHiOSUtils topVisibleViewController];
			
			UIAlertControllerStyle alertStyle = UIAlertControllerStyleAlert;
			if(UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPhone) {
				alertStyle = UIAlertControllerStyleActionSheet;
			}
			
			UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"Select Storage Provider" message:nil preferredStyle:alertStyle];
			for(auto storageProvider : storageProviders) {
				[alert addAction:[UIAlertAction actionWithTitle:storageProvider->displayName().toNSString() style:UIAlertActionStyleDefault handler:^(UIAlertAction* action) {
					storageProvider->login().then([=](bool loggedIn) {
						if(loggedIn) {
							setPrimaryStorageProvider(storageProvider);
						}
						resolve(loggedIn);
					}).except(reject);
				}]];
			}
			
			[alert addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction* action) {
				resolve(false);
			}]];
			
			[topViewController presentViewController:alert animated:YES completion:nil];
		});
	}
}

#endif

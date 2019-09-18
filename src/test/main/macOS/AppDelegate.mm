//
//  AppDelegate.mm
//  SoundHoleCoreTest-macOS
//
//  Created by Luis Finke on 9/14/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#import "AppDelegate.h"
#include <embed/nodejs/NodeJS.hpp>

@interface AppDelegate() <NodeJSProcessEventDelegate>

@end

@implementation AppDelegate

-(void)applicationDidFinishLaunching:(NSNotification*)aNotification {
	// Insert code here to initialize your application
	embed::nodejs::addProcessEventDelegate(self);
	embed::nodejs::start();
}

-(void)applicationWillTerminate:(NSNotification*)aNotification {
	// Insert code here to tear down your application
}

-(void)nodejsProcessWillStart:(NSArray<NSString*>*)args {
	NSLog(@"nodejsProcessWillStart: %@", args);
}

-(void)nodejsProcessDidStart:(napi_env)env {
	NSLog(@"nodejsProcessDidStart");
}

-(void)nodejsProcessWillEnd:(napi_env)env {
	NSLog(@"nodejsProcessWillEnd");
}

-(void)nodejsProcessDidEnd:(int)exitCode {
	NSLog(@"nodejsProcessDidEnd: %i", exitCode);
}

@end

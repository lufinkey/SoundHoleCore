//
//  ViewController.mm
//  SoundHoleCoreTest-macOS
//
//  Created by Luis Finke on 9/1/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#import "ViewController.h"
#include <test/SoundHoleCoreTest.hpp>

using namespace shtest;

@implementation ViewController

-(void)viewDidLoad {
	[super viewDidLoad];
	// Do any additional setup after loading the view.
}

-(void)viewDidAppear {
	[super viewDidAppear];
	testMetacall();
}

-(void)setRepresentedObject:(id)representedObject {
	[super setRepresentedObject:representedObject];
	// Update the view, if already loaded.
}


@end

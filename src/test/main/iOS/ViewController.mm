//
//  ViewController.mm
//  SoundHoleCoreTest-iOS
//
//  Created by Luis Finke on 9/14/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#import "ViewController.h"
#include <test/SoundHoleCoreTest.hpp>

@interface ViewController () {
	BOOL _tested;
} @end

@implementation ViewController

-(void)viewDidLoad {
	[super viewDidLoad];
	_tested = NO;
	// Do any additional setup after loading the view.
}

-(void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	_tested = YES;
	sh::test::testSpotify().then([=]() {
		return sh::test::testStreamPlayer();
	}).except([=](fgl::Error& error) {
		printf("error: %s\n", error.toString().c_str());
	}).except([=](std::exception& error) {
		printf("error: %s\n", error.what());
	});
}

@end

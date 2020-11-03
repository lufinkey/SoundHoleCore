//
//  SoundHoleCoreTest.hpp
//  SoundHoleCoreTest
//
//  Created by Luis Finke on 9/1/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/soundhole.hpp>

namespace sh::test {
	Promise<void> runTests();

	Promise<void> testBandcamp();
	Promise<void> testSpotify();
	Promise<void> testStreamPlayer();
}

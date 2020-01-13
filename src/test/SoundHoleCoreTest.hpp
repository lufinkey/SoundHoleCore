//
//  SoundHoleCoreTest.hpp
//  SoundHoleCoreTest
//
//  Created by Luis Finke on 9/1/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh::test {
	Promise<void> testSpotify();
	Promise<void> testStreamPlayer();
}

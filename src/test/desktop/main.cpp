//
//  main.cpp
//  SoundHoleCoreTest
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include <soundhole.hpp>
#include <soundhole/metacall/Metacall.hpp>

using namespace sh;

int main(int argc, char* argv[]) {
	Metacall metacall;
	metacall.loadFromFile(Metacall::Lang::NODE, "/Users/lfinke/Code/SoundHoleCore/src/soundhole/metacall/node/index.js");
	auto result = metacall.call("testFunction", {
		std::map<String,Any>{
			{ "hello", String("my brotha") },
			{ "is good", 24 }
		}
	});
	return 0;
}

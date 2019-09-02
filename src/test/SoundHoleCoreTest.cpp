//
//  SoundHoleCoreTest.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/1/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SoundHoleCoreTest.hpp"
#include <soundhole/metacall/Metacall.hpp>

namespace shtest {
	using namespace sh;
	
	void testMetacall() {
		printf("CONFIGURATION_PATH=%s\n", getenv("CONFIGURATION_PATH"));
		printf("LOADER_LIBRARY_PATH=%s\n", getenv("LOADER_LIBRARY_PATH"));
		printf("LOADER_SCRIPT_PATH=%s\n", getenv("LOADER_SCRIPT_PATH"));
		printf("DETOUR_LIBRARY_PATH=%s\n", getenv("DETOUR_LIBRARY_PATH"));
		printf("SERIAL_LIBRARY_PATH=%s\n", getenv("SERIAL_LIBRARY_PATH"));
		printf("running testMetacall\n");
		Metacall metacall;
		printf("matacall.loadFromFile\n");
		//metacall.loadFromFile(Metacall::Lang::NODE, "node/asd.js");
		metacall.loadFromString(Metacall::Lang::NODE, "function f() {\n\tconsole.log('hello');\n};");
		printf("matacall.call\n");
		auto result = metacall.call("f", {
			std::map<String,Any>{
				{ "hello", String("my brotha") },
				{ "is good", 24 }
			}
		});
		printf("done running testMetacall");
	}
}

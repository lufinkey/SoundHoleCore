//
//  metacall_core.cpp
//  metacall-core
//
//  Created by Luis Finke on 8/28/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include <metacall/metacall.h>

// dummy function to force project to compile

void __metacall_core_null_placeholder() {
	metacall_initialize();
	const char* files[] = { "null.py" };
	metacall_load_from_file("py", files, sizeof(files), nullptr);
	metacall("testFunction", 23);
	metacallv("testFunction", nullptr);
}

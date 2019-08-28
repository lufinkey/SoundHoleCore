//
//  metacall_core.cpp
//  metacall-core
//
//  Created by Luis Finke on 8/28/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include <metacall/metacall.h>

void __metacall_core_null_placeholder() {
	const char* files[] = { "null.py" };
	metacall_load_from_file("py", files, sizeof(files), nullptr);
}

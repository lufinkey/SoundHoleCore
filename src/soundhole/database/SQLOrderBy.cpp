//
//  SQLOrderBy.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 7/3/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "SQLOrderBy.hpp"

namespace sh::sql {
	String LibraryItemOrderBy_toString(LibraryItemOrderBy orderBy) {
		switch(orderBy) {
			case LibraryItemOrderBy::ADDED_AT:
				return "ADDED_AT";
			case LibraryItemOrderBy::NAME:
				return "NAME";
		}
		throw std::runtime_error("Invalid sql::LibraryItemOrderBy value");
	}

	LibraryItemOrderBy LibraryItemOrderBy_fromString(String orderBy) {
		if(orderBy == "ADDED_AT") {
			return LibraryItemOrderBy::ADDED_AT;
		}
		else if(orderBy == "NAME") {
			return LibraryItemOrderBy::NAME;
		}
		throw std::runtime_error("Invalid sql::LibraryItemOrderBy string");
	}
}

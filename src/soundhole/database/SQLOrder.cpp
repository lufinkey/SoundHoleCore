//
//  SQLOrder.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 7/3/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "SQLOrder.hpp"

namespace sh::sql {
	String Order_toString(Order order) {
		switch(order) {
			case Order::DEFAULT:
				return "DEFAULT";
			case Order::ASC:
				return "ASC";
			case Order::DESC:
				return "DESC";
		}
		throw std::runtime_error("Invalid sql::Order value");
	}

	Order Order_fromString(String order) {
		if(order == "DEFAULT") {
			return Order::DEFAULT;
		}
		else if(order == "ASC") {
			return Order::ASC;
		}
		else if(order == "DESC") {
			return Order::DESC;
		}
		throw std::runtime_error("Invalid sql::Order string");
	}
}

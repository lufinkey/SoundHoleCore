//
//  Utils.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/26/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "Utils.hpp"

namespace sh::utils {
	String getExceptionDetails(std::exception_ptr error) {
		try {
			std::rethrow_exception(error);
			return "";
		} catch(Error& error) {
			return error.toString();
		} catch(std::exception& error) {
			return error.what();
		}
	}
}

//
//  Utils.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/26/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "Utils.hpp"

namespace sh::utils {
	ExceptionDetails getExceptionDetails(std::exception_ptr error) {
		try {
			std::rethrow_exception(error);
			return {
				.fullDescription="",
				.message=""
			};
		} catch(Error& error) {
			return {
				.fullDescription = error.toString(),
				.message = error.getMessage()
			};
		} catch(std::exception& error) {
			return {
				.fullDescription=error.what(),
				.message=error.what()
			};
		}
	}
}

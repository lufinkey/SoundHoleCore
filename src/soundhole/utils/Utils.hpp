//
//  Utils.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/26/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>

namespace sh::utils {
	struct ExceptionDetails {
		String fullDescription;
		String message;
	};
	ExceptionDetails getExceptionDetails(std::exception_ptr error);
}

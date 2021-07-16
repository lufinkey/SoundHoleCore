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
		} catch(Error& e) {
			return {
				.fullDescription = e.toString(),
				.message = e.getMessage()
			};
		} catch(std::exception& e) {
			return {
				.fullDescription=e.what(),
				.message=e.what()
			};
		}
	}

	#if !defined(TARGETPLATFORM_IOS) && !defined(TARGETPLATFORM_MAC)

	String getTmpDirectoryPath() {
		return fs::temporaryDirectory();
	}

	String getCacheDirectoryPath() {
		return fs::temporaryDirectory();
	}

	#endif
}

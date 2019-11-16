//
//  YoutubeError.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "YoutubeError.hpp"

namespace sh {
	String YoutubeError::Code_toString(Code code) {
		#define CASE_CODE(codeEnum) \
			case Code::codeEnum: \
				return #codeEnum;

		switch(code) {
			CASE_CODE(NOT_INITIALIZED)
			CASE_CODE(REQUEST_FAILED)
			CASE_CODE(BAD_DATA)
		}
		
		#undef CASE_CODE
	}


	YoutubeError::YoutubeError(Code code, String message)
	: code(code), message(message) {
		//
	}

	YoutubeError::Code YoutubeError::getCode() const {
		return code;
	}
	
	String YoutubeError::getMessage() const {
		return message;
	}

	String YoutubeError::toString() const {
		return "YoutubeError[" + Code_toString(code) + "]: " + getMessage();
	}
}

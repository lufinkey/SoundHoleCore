//
//  BandcampError.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 10/16/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "BandcampError.hpp"

namespace sh {
	String BandcampError::Code_toString(Code code) {
		#define CASE_CODE(codeEnum) \
			case Code::codeEnum: \
				return #codeEnum;

		switch(code) {
			CASE_CODE(NOT_INITIALIZED)
			CASE_CODE(REQUEST_FAILED)
			CASE_CODE(BAD_RESPONSE)
		}
		
		#undef CASE_CODE
	}


	BandcampError::BandcampError(Code code, String message)
	: code(code), message(message) {
		//
	}

	BandcampError::Code BandcampError::getCode() const {
		return code;
	}
	
	String BandcampError::getMessage() const {
		return message;
	}

	String BandcampError::toString() const {
		return "BandcampError: " + getMessage();
	}
}

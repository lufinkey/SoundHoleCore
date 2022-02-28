//
//  MusicBrainzError.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 2/27/22.
//  Copyright © 2022 Luis Finke. All rights reserved.
//

#include "MusicBrainzError.hpp"

namespace sh {
	String MusicBrainzError::Code_toString(Code code) {
		#define CASE_CODE(codeEnum) \
			case Code::codeEnum: \
				return #codeEnum;

		switch(code) {
			CASE_CODE(REQUEST_FAILED),
			CASE_CODE(BAD_DATA)
		}
		
		#undef CASE_CODE
	}


	MusicBrainzError::MusicBrainzError(Code code, String message)
	: code(code), message(message) {
		//
	}

	MusicBrainzError::Code MusicBrainzError::getCode() const {
		return code;
	}

	String MusicBrainzError::getMessage() const {
		return message;
	}

	String MusicBrainzError::toString() const {
		return "MusicBrainzError[" + Code_toString(code) + "]: " + getMessage();
	}

	const char* MusicBrainzError::what() const noexcept {
		return message.c_str();
	}
}

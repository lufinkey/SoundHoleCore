//
//  LastFMError.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/5/21.
//  Copyright © 2021 Luis Finke. All rights reserved.
//

#include "LastFMError.hpp"

namespace sh {
	String LastFMError::Code_toString(Code code) {
		#define CASE_CODE(codeEnum) \
			case Code::codeEnum: \
				return #codeEnum;

		switch(code) {
			CASE_CODE(REQUEST_FAILED)
			CASE_CODE(BAD_DATA)
			CASE_CODE(NOT_LOGGED_IN)
		}
		
		#undef CASE_CODE
	}


	LastFMError::LastFMError(Code code, String message, Map<String,Any> details)
	: code(code), message(message), details(details) {
		//
	}

	LastFMError::Code LastFMError::getCode() const {
		return code;
	}

	const Map<String,Any>& LastFMError::getDetails() const {
		return details;
	}

	Any LastFMError::getDetail(const String& key) const {
		return details.get(key, Any());
	}

	String LastFMError::getMessage() const {
		return message;
	}

	String LastFMError::toString() const {
		return "LastFMError[" + Code_toString(code) + "]: " + getMessage();
	}

	const char* LastFMError::what() const noexcept {
		return message.c_str();
	}
}

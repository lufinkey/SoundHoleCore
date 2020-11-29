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
			CASE_CODE(REQUEST_NOT_SENT)
			CASE_CODE(REQUEST_FAILED)
			CASE_CODE(SESSION_EXPIRED)
			CASE_CODE(BAD_DATA)
			CASE_CODE(NOT_FOUND)
			CASE_CODE(OAUTH_REQUEST_FAILED)
		}
		
		#undef CASE_CODE
	}


	YoutubeError::YoutubeError(Code code, String message, std::map<String,Any> details)
	: code(code), message(message), details(details) {
		//
	}

	YoutubeError::Code YoutubeError::getCode() const {
		return code;
	}

	const std::map<String,Any>& YoutubeError::getDetails() const {
		return details;
	}

	Any YoutubeError::getDetail(const String& key) const {
		try {
			return details.at(key);
		} catch(std::out_of_range&) {
			return Any();
		}
	}
	
	String YoutubeError::getMessage() const {
		return message;
	}

	String YoutubeError::toString() const {
		return "YoutubeError[" + Code_toString(code) + "]: " + getMessage();
	}

	const char* YoutubeError::what() const noexcept {
		return message.c_str();
	}
}

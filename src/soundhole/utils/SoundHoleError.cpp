//
//  SoundHoleError.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/20/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SoundHoleError.hpp"
#include <stdexcept>

namespace sh {
	String SoundHoleError::Code_toString(Code code) {
		switch(code) {
			case Code::REQUEST_NOT_SENT:
				return "REQUEST_NOT_SENT";
			case Code::REQUEST_FAILED:
				return "REQUEST_FAILED";
		}
		throw std::invalid_argument("invalid enum value for sh::SoundHoleError::Code");
	}
	
	SoundHoleError::SoundHoleError(Code code, String message)
	: code(code), message(message) {
		//
	}
	
	SoundHoleError::Code SoundHoleError::getCode() const {
		return code;
	}
	
	String SoundHoleError::getCodeName() const {
		return Code_toString(code);
	}
	
	String SoundHoleError::getMessage() const {
		return message;
	}
	
	String SoundHoleError::toString() const {
		return "SoundHoleError[" + getCodeName() + "]: " + message;
	}
}

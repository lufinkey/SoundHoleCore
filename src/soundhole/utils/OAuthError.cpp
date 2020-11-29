//
//  OAuthError.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/13/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "OAuthError.hpp"

namespace sh {
	String OAuthError::Code_toString(Code code) {
		#define CASE_CODE(codeEnum) \
			case Code::codeEnum: \
				return #codeEnum;

		switch(code) {
			CASE_CODE(REQUEST_NOT_SENT)
			CASE_CODE(REQUEST_FAILED)
			CASE_CODE(BAD_DATA)
			CASE_CODE(INVALID_REQUEST)
			CASE_CODE(INVALID_CLIENT)
			CASE_CODE(INVALID_GRANT)
			CASE_CODE(INVALID_SCOPE)
			CASE_CODE(UNAUTHORIZED_CLIENT)
			CASE_CODE(UNSUPPORTED_GRANT_TYPE)
			CASE_CODE(SESSION_EXPIRED)
			CASE_CODE(VERIFIER_MISMATCH)
		}
		
		#undef CASE_CODE
	}


	Optional<OAuthError::Code> OAuthError::Code_fromOAuthError(String error) {
		if(error == "invalid_request") {
			return Code::INVALID_REQUEST;
		} else if(error == "invalid_client") {
			return Code::INVALID_CLIENT;
		} else if(error == "invalid_grant") {
			return Code::INVALID_GRANT;
		} else if(error == "invalid_scope") {
			return Code::INVALID_SCOPE;
		} else if(error == "unauthorized_client") {
			return Code::UNAUTHORIZED_CLIENT;
		} else if(error == "unsupported_grant_type") {
			return Code::UNSUPPORTED_GRANT_TYPE;
		}
		return std::nullopt;
	}


	String OAuthError::Code_getDescription(Code code) {
		switch(code) {
			case Code::REQUEST_NOT_SENT:
				return "Request could not be sent";
			case Code::REQUEST_FAILED:
				return "Request failed";
			case Code::BAD_DATA:
				return "Invalid data";
			case Code::INVALID_REQUEST:
				return "Invalid request";
			case Code::INVALID_CLIENT:
				return "Invalid client";
			case Code::INVALID_GRANT:
				return "Invalid grant";
			case Code::INVALID_SCOPE:
				return "Invalid scope";
			case Code::UNAUTHORIZED_CLIENT:
				return "Unauthorized client";
			case Code::UNSUPPORTED_GRANT_TYPE:
				return "Unsupported grant type";
			case Code::SESSION_EXPIRED:
				return "Session is expired";
			case Code::VERIFIER_MISMATCH:
				return "Verifier mismatch";
		}
		return "Unknown error";
	}


	OAuthError::OAuthError(Code code, String message, std::map<String,Any> details)
	: code(code), message(message), details(details) {
		//
	}

	OAuthError::Code OAuthError::getCode() const {
		return code;
	}

	const std::map<String,Any>& OAuthError::getDetails() const {
		return details;
	}

	Any OAuthError::getDetail(const String& key) const {
		try {
			return details.at(key);
		} catch(std::out_of_range&) {
			return Any();
		}
	}
	
	String OAuthError::getMessage() const {
		return message;
	}

	String OAuthError::toString() const {
		return "OAuthError[" + Code_toString(code) + "]: " + getMessage();
	}

	const char* OAuthError::what() const noexcept {
		return message.c_str();
	}
}

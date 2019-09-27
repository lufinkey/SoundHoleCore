//
//  SpotifyError.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/20/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SpotifyError.hpp"

namespace sh {
	String SpotifyError::Code_toString(Code code) {
		#define CASE_CODE(codeEnum) \
			case Code::codeEnum: \
				return #codeEnum;

		switch(code) {
			CASE_CODE(BAD_PARAMETERS)
			CASE_CODE(REQUEST_NOT_SENT)
			CASE_CODE(REQUEST_FAILED)
			CASE_CODE(BAD_RESPONSE)
			CASE_CODE(NOT_LOGGED_IN)
			CASE_CODE(SESSION_EXPIRED)
				
			CASE_CODE(OAUTH_REQUEST_FAILED)
			CASE_CODE(OAUTH_STATE_MISMATCH)
				
			CASE_CODE(SDK_FAILED)
			CASE_CODE(SDK_INIT_FAILED)
			CASE_CODE(SDK_WRONG_API_VERSION)
			CASE_CODE(SDK_NULL_ARGUMENT)
			CASE_CODE(SDK_INVALID_ARGUMENT)
			CASE_CODE(SDK_UNINITIALIZED)
			CASE_CODE(SDK_ALREADY_INITIALIZED)
			CASE_CODE(SDK_LOGIN_BAD_CREDENTIALS)
			CASE_CODE(SDK_NEEDS_PREMIUM)
			CASE_CODE(SDK_TRAVEL_RESTRICTION)
			CASE_CODE(SDK_APPLICATION_BANNED)
			CASE_CODE(SDK_GENERAL_LOGIN_ERROR)
			CASE_CODE(SDK_UNSUPPORTED)
			CASE_CODE(SDK_NOT_ACTIVE_DEVICE)
			CASE_CODE(SDK_API_RATE_LIMITED)
			CASE_CODE(SDK_PLAYBACK_START_FAILED)
			CASE_CODE(SDK_GENERAL_PLAYBACK_ERROR)
			CASE_CODE(SDK_PLAYBACK_RATE_LIMITED)
			CASE_CODE(SDK_PLAYBACK_CAPPING_LIMIT_REACHED)
			CASE_CODE(SDK_AD_IS_PLAYING)
			CASE_CODE(SDK_CORRUPT_TRACK)
			CASE_CODE(SDK_CONTEXT_FAILED)
			CASE_CODE(SDK_PREFETCH_ITEM_UNAVAILABLE)
			CASE_CODE(SDK_ALREADY_PREFETCHING)
			CASE_CODE(SDK_STORAGE_WRITE_ERROR)
			CASE_CODE(SDK_PREFETCH_DOWNLOAD_FAILED)
		}
		
		#undef CASE_CODE
	}
	
	
	SpotifyError::SpotifyError(Code code, String message, std::map<String,Any> details)
	: code(code), message(message), details(details) {
		//
	}
	
	SpotifyError::Code SpotifyError::getCode() const {
		return code;
	}
	
	const std::map<String,Any>& SpotifyError::getDetails() const {
		return details;
	}
	
	Any SpotifyError::getDetail(const String& key) const {
		try {
			return details.at(key);
		} catch(std::out_of_range&) {
			return Any();
		}
	}
	
	String SpotifyError::getMessage() const {
		return message;
	}
	
	String SpotifyError::toString() const {
		return "SpotifyError[" + Code_toString(code) + "]: " + getMessage();
	}
}

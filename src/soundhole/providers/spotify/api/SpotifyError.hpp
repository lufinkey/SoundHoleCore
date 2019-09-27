//
//  SpotifyError.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/20/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <map>
#ifdef __OBJC__
#import <Foundation/Foundation.h>
#endif
#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <SpotifyAudioPlayback/SpotifyAudioPlayback.h>
#endif

namespace sh {
	class SpotifyError: public Error {
	public:
		enum class Code {
			BAD_PARAMETERS,
			REQUEST_NOT_SENT,
			REQUEST_FAILED,
			BAD_RESPONSE,
			NOT_LOGGED_IN,
			SESSION_EXPIRED,
			
			OAUTH_REQUEST_FAILED,
			OAUTH_STATE_MISMATCH,
			
			SDK_FAILED,
			SDK_INIT_FAILED,
			SDK_WRONG_API_VERSION,
			SDK_NULL_ARGUMENT,
			SDK_INVALID_ARGUMENT,
			SDK_UNINITIALIZED,
			SDK_ALREADY_INITIALIZED,
			SDK_LOGIN_BAD_CREDENTIALS,
			SDK_NEEDS_PREMIUM,
			SDK_TRAVEL_RESTRICTION,
			SDK_APPLICATION_BANNED,
			SDK_GENERAL_LOGIN_ERROR,
			SDK_UNSUPPORTED,
			SDK_NOT_ACTIVE_DEVICE,
			SDK_API_RATE_LIMITED,
			SDK_PLAYBACK_START_FAILED,
			SDK_GENERAL_PLAYBACK_ERROR,
			SDK_PLAYBACK_RATE_LIMITED,
			SDK_PLAYBACK_CAPPING_LIMIT_REACHED,
			SDK_AD_IS_PLAYING,
			SDK_CORRUPT_TRACK,
			SDK_CONTEXT_FAILED,
			SDK_PREFETCH_ITEM_UNAVAILABLE,
			SDK_ALREADY_PREFETCHING,
			SDK_STORAGE_WRITE_ERROR,
			SDK_PREFETCH_DOWNLOAD_FAILED
		};
		static String Code_toString(Code code);
		
		SpotifyError(Code code, String message, std::map<String,Any> details = {});
		
		#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
		SpotifyError(NSError* error);
		#endif
		
		Code getCode() const;
		const std::map<String,Any>& getDetails() const;
		Any getDetail(const String& key) const;
		
		virtual String getMessage() const override;
		virtual String toString() const override;
		
	private:
		Code code;
		String message;
		std::map<String,Any> details;
	};
}

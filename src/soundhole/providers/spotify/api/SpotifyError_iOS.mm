//
//  SpotifyError_iOS.m
//  SoundHoleCore
//
//  Created by Luis Finke on 9/21/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SpotifyError.hpp"

#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <SpotifyAudioPlayback/SpotifyAudioPlayback.h>
#import <Foundation/Foundation.h>

namespace sh {
	SpotifyError::Code SpotifyError_codeFromSDKError(NSError* error) {
		#define CODE_CASE(sdkErrorType, mapErrorType) \
			case sdkErrorType: \
				return SpotifyError::Code::mapErrorType;
		
		if([error.domain isEqualToString:@"com.spotify.ios-sdk.playback"]) {
			switch((SpErrorCode)error.code) {
				case SPErrorOk:
					NSLog(@"Warning: got SpErrorOk code inside error");
					return SpotifyError::Code::SDK_FAILED;
				CODE_CASE(SPErrorFailed, SDK_FAILED)
				CODE_CASE(SPErrorInitFailed, SDK_INIT_FAILED)
				CODE_CASE(SPErrorWrongAPIVersion, SDK_WRONG_API_VERSION)
				CODE_CASE(SPErrorNullArgument, SDK_NULL_ARGUMENT)
				CODE_CASE(SPErrorInvalidArgument, SDK_INVALID_ARGUMENT)
				CODE_CASE(SPErrorUninitialized, SDK_UNINITIALIZED)
				CODE_CASE(SPErrorAlreadyInitialized, SDK_ALREADY_INITIALIZED)
				CODE_CASE(SPErrorLoginBadCredentials, SDK_LOGIN_BAD_CREDENTIALS)
				CODE_CASE(SPErrorNeedsPremium, SDK_NEEDS_PREMIUM)
				CODE_CASE(SPErrorTravelRestriction, SDK_TRAVEL_RESTRICTION)
				CODE_CASE(SPErrorApplicationBanned, SDK_APPLICATION_BANNED)
				CODE_CASE(SPErrorGeneralLoginError, SDK_GENERAL_LOGIN_ERROR)
				CODE_CASE(SPErrorUnsupported, SDK_UNSUPPORTED)
				CODE_CASE(SPErrorNotActiveDevice, SDK_NOT_ACTIVE_DEVICE)
				CODE_CASE(SPErrorAPIRateLimited, SDK_API_RATE_LIMITED)
				CODE_CASE(SPErrorPlaybackErrorStart, SDK_PLAYBACK_START_FAILED)
				CODE_CASE(SPErrorGeneralPlaybackError, SDK_GENERAL_PLAYBACK_ERROR)
				CODE_CASE(SPErrorPlaybackRateLimited, SDK_PLAYBACK_RATE_LIMITED)
				CODE_CASE(SPErrorPlaybackCappingLimitReached, SDK_PLAYBACK_CAPPING_LIMIT_REACHED)
				CODE_CASE(SPErrorAdIsPlaying, SDK_AD_IS_PLAYING)
				CODE_CASE(SPErrorCorruptTrack, SDK_CORRUPT_TRACK)
				CODE_CASE(SPErrorContextFailed, SDK_CONTEXT_FAILED)
				CODE_CASE(SPErrorPrefetchItemUnavailable, SDK_PREFETCH_ITEM_UNAVAILABLE)
				CODE_CASE(SPAlreadyPrefetching, SDK_ALREADY_PREFETCHING)
				CODE_CASE(SPStorageWriteError, SDK_STORAGE_WRITE_ERROR)
				CODE_CASE(SPPrefetchDownloadFailed, SDK_PREFETCH_DOWNLOAD_FAILED)
			}
			NSLog(@"Warning: unknown SpErrorCode value for error %@", error);
			return SpotifyError::Code::SDK_FAILED;
		}
		else {
			NSLog(@"Warning: failed to get error code for error %@", error);
			return SpotifyError::Code::SDK_FAILED;
		}
		
		#undef CODE_CASE
	}
	
	SpotifyError::SpotifyError(NSError* error)
	: code(SpotifyError_codeFromSDKError(error)), message(error.localizedDescription) {
		//
	}
}

#endif


#include <soundhole/jnicpp/android/spotify/SpotifyError_jni.hpp>

#ifdef SOUNDHOLE_JNI_ENABLED
namespace sh::jni::android::spotify {
	void SpotifyError_generateEnumMap(JNIEnv* env);

	FGL_JNI_DEF_JCLASS(SpotifyError, "com/spotify/sdk/android/player/Error")
	FGL_JNI_DEF_JFIELD(SpotifyError, nativeCode, "nativeCode", "I")
	void SpotifyError::init(JNIEnv* env) {
		javaClass(env);
		fieldID_nativeCode(env);
		SpotifyError_generateEnumMap(env);
	}

	std::map<jint,std::tuple<sh::SpotifyError::Code,String>> enumMap = {};
	void SpotifyError_generateEnumMap(JNIEnv* env) {
		using Code = sh::SpotifyError::Code;
		auto getEntry = [=](const char* javaErrorCodeName, Code cppErrorCode, String message) -> std::pair<int,std::tuple<Code,String>> {
			auto enumVal = SpotifyError::getEnum(env, javaErrorCodeName);
			jint javaErrorCode = enumVal.getNativeCode(env);
			return { javaErrorCode, { cppErrorCode, message } };
		};
		enumMap = {
			getEntry("UNKNOWN", Code::SDK_FAILED, "The operation failed due to an unknown issue."),
			getEntry("kSpErrorOk", Code::SDK_FAILED, "The operation was successful. I don't know why this is an error..."),
			getEntry("kSpErrorFailed", Code::SDK_FAILED, "The operation failed due to an unspecified issue."),
			getEntry("kSpErrorInitFailed", Code::SDK_INIT_FAILED, "Audio streaming could not be initialized."),
			getEntry("kSpErrorWrongAPIVersion", Code::SDK_WRONG_API_VERSION, "Audio streaming could not be initialized because of an incompatible API version."),
			getEntry("kSpErrorNullArgument", Code::SDK_NULL_ARGUMENT, "An unexpected NULL pointer was passed as an argument to a function."),
			getEntry("kSpErrorInvalidArgument", Code::SDK_INVALID_ARGUMENT, "An unexpected argument value was passed to a function."),
			getEntry("kSpErrorUninitialized", Code::SDK_UNINITIALIZED, "Audio streaming has not yet been initialised for this application."),
			getEntry("kSpErrorAlreadyInitialized", Code::SDK_ALREADY_INITIALIZED, "Audio streaming has already been initialised for this application."),
			getEntry("kSpErrorLoginBadCredentials", Code::SDK_LOGIN_BAD_CREDENTIALS, "Login to Spotify failed because of invalid credentials."),
			getEntry("kSpErrorNeedsPremium", Code::SDK_NEEDS_PREMIUM, "This operation requires a Spotify Premium account."),
			getEntry("kSpErrorTravelRestriction", Code::SDK_TRAVEL_RESTRICTION, "Spotify user is not allowed to log in from this country."),
			getEntry("kSpErrorApplicationBanned", Code::SDK_APPLICATION_BANNED, "This application has been banned by Spotify."),
			getEntry("kSpErrorGeneralLoginError", Code::SDK_GENERAL_LOGIN_ERROR, "An unspecified login error occurred."),
			getEntry("kSpErrorUnsupported", Code::SDK_UNSUPPORTED, "The operation is not supported."),
			getEntry("kSpErrorNotActiveDevice", Code::SDK_NOT_ACTIVE_DEVICE, "The operation is not supported if the device is not the active playback device."),
			getEntry("kSpErrorAPIRateLimited", Code::SDK_API_RATE_LIMITED, "This application has made too many API requests at a time, so it is now rate-limited."),
			getEntry("kSpErrorPlaybackErrorStart", Code::SDK_PLAYBACK_START_FAILED, "Unable to start playback."),
			getEntry("kSpErrorGeneralPlaybackError", Code::SDK_GENERAL_PLAYBACK_ERROR, "An unspecified playback error occurred."),
			getEntry("kSpErrorPlaybackRateLimited", Code::SDK_PLAYBACK_RATE_LIMITED, "This application has requested track playback too many times, so it is now rate-limited."),
			getEntry("kSpErrorPlaybackCappingLimitReached", Code::SDK_PLAYBACK_CAPPING_LIMIT_REACHED, "This application's playback limit has been reached."),
			getEntry("kSpErrorAdIsPlaying", Code::SDK_AD_IS_PLAYING, "An ad is playing."),
			getEntry("kSpErrorCorruptTrack", Code::SDK_CORRUPT_TRACK, "The track is corrupted."),
			getEntry("kSpErrorContextFailed", Code::SDK_CONTEXT_FAILED, "The operation failed."),
			getEntry("kSpErrorPrefetchItemUnavailable", Code::SDK_PREFETCH_ITEM_UNAVAILABLE, "Item is unavailable for pre-fetch."),
			getEntry("kSpAlreadyPrefetching", Code::SDK_ALREADY_PREFETCHING, "Item is already pre-fetching."),
			getEntry("kSpStorageReadError", Code::SDK_STORAGE_READ_ERROR, "Storage read failed."),
			getEntry("kSpStorageWriteError", Code::SDK_STORAGE_WRITE_ERROR, "Storage write failed."),
			getEntry("kSpPrefetchDownloadFailed", Code::SDK_PREFETCH_DOWNLOAD_FAILED, "Pre-fetch download failed.")
		};
	}

	SpotifyError SpotifyError::getEnum(JNIEnv* env, const char* name) {
		auto cls = javaClass(env);
		jfieldID fieldId = env->GetStaticFieldID(cls, name, "Lcom/spotify/sdk/android/player/Error;");
		return SpotifyError(env->GetStaticObjectField(cls, fieldId));
	}

	jint SpotifyError::getNativeCode(JNIEnv* env) {
		return env->GetIntField(value, fieldID_nativeCode(env));
	}
}
#endif


#pragma once

#ifdef __ANDROID__
#define SOUNDHOLE_JNI_ENABLED
#endif

#ifdef SOUNDHOLE_JNI_ENABLED

#include <jni.h>
#include <soundhole/common.hpp>
#include <fgl/async/JNIAsyncCpp.hpp>

namespace sh {
	struct JNIObject {
	public:
		JNIObject();
		explicit JNIObject(jobject val);
		JNIObject& operator=(jobject val);
		JNIObject& operator=(std::nullptr_t);

		operator jobject() const;

		jobject value;
	};
}

#endif

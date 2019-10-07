//
//  SpotifySession_iOS.mm
//  SoundHoleCore
//
//  Created by Luis Finke on 9/22/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#ifdef __ANDROID__

#include <jni.h>
#include "SpotifySession.hpp"
#include <soundhole/utils/android/AndroidUtils.hpp>
#include <embed/nodejs/NodeJS_jni.hpp>

#ifdef TARGETPLATFORM_ANDROID

namespace sh {
	using ScopedJNIEnv = embed::nodejs::ScopedJNIEnv;

	Optional<SpotifySession> SpotifySession::load(const String& key) {
		ScopedJNIEnv scopedEnv(getMainJavaVM());
		JNIEnv* env = scopedEnv.getEnv();
		jobject context = getMainAndroidContext(env);
		if(context == nullptr) {
			throw std::runtime_error("SoundHole.mainContext has not been set");
		}
		return fromAndroidSharedPrefs(env, key, context);
	}

	void SpotifySession::save(const String& key, Optional<SpotifySession> session) {
		ScopedJNIEnv scopedEnv(getMainJavaVM());
		JNIEnv* env = scopedEnv.getEnv();
		jobject context = getMainAndroidContext(env);
		if(context == nullptr) {
			throw std::runtime_error("SoundHole.mainContext has not been set");
		}
		writeToAndroidSharedPrefs(env, session, key, context);
	}

	void SpotifySession::save(const String& key) {
		ScopedJNIEnv scopedEnv(getMainJavaVM());
		JNIEnv* env = scopedEnv.getEnv();
		jobject context = getMainAndroidContext(env);
		if(context == nullptr) {
			throw std::runtime_error("SoundHole.mainContext has not been set");
		}
		writeToAndroidSharedPrefs(env, key, context);
	}

	jobject SpotifySession_android_Context_getSharedPreferences(JNIEnv* env, const String& key, jobject context) {
		jclass contextClass = env->GetObjectClass(context);
		jmethodID context_getSharedPrefs = env->GetMethodID(contextClass, "getSharedPreferences", "(Ljava/lang/String;I)Landroid/content/SharedPreferences;");
		jfieldID context_modePrivate = env->GetStaticFieldID(contextClass, "MODE_PRIVATE", "I");
		if(context_getSharedPrefs == nullptr) {
			throw std::runtime_error("Could not find android Context.getSharedPreferences");
		} else if(context_modePrivate == nullptr) {
			throw std::runtime_error("Could not find android Context.MODE_PRIVATE");
		}
		jstring sessionKey = env->NewStringUTF(key);
		jint modePrivate = env->GetStaticIntField(contextClass, context_modePrivate);
		return env->CallObjectMethod(context, context_getSharedPrefs, sessionKey, modePrivate);
	}

	jobject SpotifySession_android_SharedPreferences_edit(JNIEnv* env, jobject sharedPrefs) {
		jclass sharedPrefsClass = env->GetObjectClass(sharedPrefs);
		jmethodID sharedPrefs_edit = env->GetMethodID(sharedPrefsClass, "edit", "()Landroid/content/SharedPreferences$Editor;");
		if(sharedPrefs_edit == nullptr) {
			throw std::runtime_error("Could not find android SharedPreferences.edit");
		}
		return env->CallObjectMethod(sharedPrefs, sharedPrefs_edit);
	}

	void SpotifySession::writeToAndroidSharedPrefs(JNIEnv* env, const String& key, jobject context) const {
		jobject sharedPrefs = SpotifySession_android_Context_getSharedPreferences(env, key, context);
		jobject prefsEditor = SpotifySession_android_SharedPreferences_edit(env, sharedPrefs);

		jclass prefsEditorClass = env->GetObjectClass(prefsEditor);
		jmethodID prefsEditor_putString = env->GetMethodID(prefsEditorClass, "putString", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/SharedPreferences$Editor;");
		jmethodID prefsEditor_putLong = env->GetMethodID(prefsEditorClass, "putLong", "(Ljava/lang/String;J)Landroid/content/SharedPreferences$Editor;");
		jmethodID prefsEditor_remove = env->GetMethodID(prefsEditorClass, "remove", "(Ljava/lang/String;)Landroid/content/SharedPreferences$Editor;");
		jmethodID prefsEditor_apply = env->GetMethodID(prefsEditorClass, "apply", "()V");
		if(prefsEditor_putString == nullptr) {
			throw std::runtime_error("Could not find android SharedPreferences.Editor.putString");
		} else if(prefsEditor_putLong == nullptr) {
			throw std::runtime_error("Could not find android SharedPreferences.Editor.putLong");
		} else if(prefsEditor_remove == nullptr) {
			throw std::runtime_error("Could not find android SharedPreferences.Editor.remove");
		} else if(prefsEditor_apply == nullptr) {
			throw std::runtime_error("Could not find android SharedPreferences.Editor.apply");
		}

		env->CallObjectMethod(prefsEditor, prefsEditor_putString,
			env->NewStringUTF("accessToken"), accessToken.toJavaString(env));
		env->CallObjectMethod(prefsEditor, prefsEditor_putLong,
			env->NewStringUTF("expireTime"), (jlong)std::chrono::system_clock::to_time_t(expireTime));
		if(refreshToken.empty()) {
			env->CallObjectMethod(prefsEditor, prefsEditor_remove,
				env->NewStringUTF("refreshToken"));
		} else {
			env->CallObjectMethod(prefsEditor, prefsEditor_putString,
				env->NewStringUTF("refreshToken"), refreshToken.toJavaString(env));
		}
		env->CallObjectMethod(prefsEditor, prefsEditor_putString,
			env->NewStringUTF("scopes"), String::join(scopes, ",").toJavaString(env));

		env->CallVoidMethod(prefsEditor, prefsEditor_apply);
	}

	void SpotifySession::writeToAndroidSharedPrefs(JNIEnv* env, const Optional<SpotifySession>& session, const String& key, jobject context) {
		if(session) {
			session->writeToAndroidSharedPrefs(env, key, context);
		} else {
			jobject sharedPrefs = SpotifySession_android_Context_getSharedPreferences(env, key, context);
			jobject prefsEditor = SpotifySession_android_SharedPreferences_edit(env, sharedPrefs);
			jclass prefsEditorClass = env->GetObjectClass(prefsEditor);
			jmethodID prefsEditor_clear = env->GetMethodID(prefsEditorClass, "clear", "()Landroid/content/SharedPreferences$Editor;");
			jmethodID prefsEditor_apply = env->GetMethodID(prefsEditorClass, "apply", "()V");
			if(prefsEditor_clear == nullptr) {
				throw std::runtime_error("Could not find android SharedPreferences.Editor.clear");
			} else if(prefsEditor_apply == nullptr) {
				throw std::runtime_error("Could not find android SharedPreferences.Editor.apply");
			}
			env->CallObjectMethod(prefsEditor, prefsEditor_clear);
			env->CallVoidMethod(prefsEditor, prefsEditor_apply);
		}
	}

	Optional<SpotifySession> SpotifySession::fromAndroidSharedPrefs(JNIEnv* env, const String& key, jobject context) {
		jobject sharedPrefs = SpotifySession_android_Context_getSharedPreferences(env, key, context);
		jclass sharedPrefsClass = env->GetObjectClass(sharedPrefs);
		jmethodID sharedPrefs_getString = env->GetMethodID(sharedPrefsClass, "getString", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
		jmethodID sharedPrefs_getLong = env->GetMethodID(sharedPrefsClass, "getLong", "(Ljava/lang/String;J)J");
		if(sharedPrefs_getString == nullptr) {
			throw std::runtime_error("Could not find android SharedPreferences.getString");
		} else if(sharedPrefs_getLong == nullptr) {
			throw std::runtime_error("Could not find android SharedPreferences.getLong");
		}
		jstring accessToken = (jstring)env->CallObjectMethod(sharedPrefs, sharedPrefs_getString, env->NewStringUTF("accessToken"), (jstring)nullptr);
		jlong expireTime = env->CallLongMethod(sharedPrefs, sharedPrefs_getLong, env->NewStringUTF("expireTime"), (jlong)0);
		if(accessToken == nullptr || expireTime == 0) {
			return std::nullopt;
		}
		jstring refreshToken = (jstring)env->CallObjectMethod(sharedPrefs, sharedPrefs_getString, env->NewStringUTF("refreshToken"), (jstring)nullptr);
		jstring scope = (jstring)env->CallObjectMethod(sharedPrefs, sharedPrefs_getString, env->NewStringUTF("scopes"), (jstring)nullptr);
		return SpotifySession(
				String(env, accessToken),
				std::chrono::system_clock::from_time_t((time_t)expireTime),
				(refreshToken != nullptr) ? String(env, refreshToken) : String(),
				(scope != nullptr) ? String(env, scope).split(',').where([](auto& str) { return !str.empty(); }) : LinkedList<String>());
	}
}

#endif
#endif

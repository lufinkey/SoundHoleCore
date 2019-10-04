#include <jni.h>
#include <string>

#include <soundhole.hpp>
#include <test/SoundHoleCoreTest.hpp>

extern "C" JNIEXPORT void JNICALL
Java_com_lufinkey_soundholecoretest_MainActivity_testSpotify(
		JNIEnv *env,
		jobject /* this */) {
	sh::test::testSpotify();
}

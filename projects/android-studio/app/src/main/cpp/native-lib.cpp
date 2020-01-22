#include <jni.h>
#include <string>

#include <soundhole/soundhole.hpp>
#include <test/SoundHoleCoreTest.hpp>

extern "C" JNIEXPORT void JNICALL
Java_com_lufinkey_soundholecoretest_MainActivity_runTests(
		JNIEnv *env,
		jobject /* this */) {
	sh::test::testSpotify().then([]() {
		sh::test::testStreamPlayer();
	});
}

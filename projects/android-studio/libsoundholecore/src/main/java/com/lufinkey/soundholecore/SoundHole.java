package com.lufinkey.soundholecore;

import android.app.Activity;
import android.content.Context;

public class SoundHole {
	static {
		staticInit();
	}
	private static native void staticInit();

	private static Activity mainActivity;
	public static Activity getMainActivity() {
		return mainActivity;
	}
	public static Context getAppContext() {
		return mainActivity.getApplicationContext();
	}
}

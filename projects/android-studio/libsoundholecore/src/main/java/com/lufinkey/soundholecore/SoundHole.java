package com.lufinkey.soundholecore;

import android.app.Activity;
import android.content.Context;

public class SoundHole {
	static {
		staticInit();
	}
	private static native void staticInit();

	private static Context appContext;
	public static Context getAppContext() {
		return appContext;
	}

	public static void init(Context context) {
		appContext = context;
	}
}

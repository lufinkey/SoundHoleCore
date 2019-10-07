package com.lufinkey.soundholecore;

import android.content.Context;

public class SoundHole {
	static {
		staticInit();
	}

	private static Context mainContext;
	public static Context getMainContext() {
		return mainContext;
	}

	private static native void staticInit();
}

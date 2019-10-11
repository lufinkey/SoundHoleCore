package com.lufinkey.soundholecore;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;

public class SoundHole {
	static {
		staticInit();
	}
	private static native void staticInit();

	public static void init(Context context) {
		Utils.appContext = context;
	}
}

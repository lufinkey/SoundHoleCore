package com.lufinkey.soundholecore;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;

public class SoundHole {
	static {
		staticInit();
	}
	private static native void staticInit();

	private static Context appContext;
	public static Context getAppContext() {
		return appContext;
	}
	public static void runOnMainThread(Runnable runnable) {
		(new Handler(Looper.getMainLooper())).post(runnable);
	}
	public static void runOnMainThread(NativeFunction function) {
		final NativeFunction func = function;
		(new Handler(Looper.getMainLooper())).post(new Runnable() {
			@Override
			public void run() {
				func.call();
			}
		});
	}

	public static void init(Context context) {
		appContext = context;
	}
}

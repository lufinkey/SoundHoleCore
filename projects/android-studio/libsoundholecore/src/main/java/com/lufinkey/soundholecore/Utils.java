package com.lufinkey.soundholecore;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;

class Utils {
	public static Context appContext;
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
}

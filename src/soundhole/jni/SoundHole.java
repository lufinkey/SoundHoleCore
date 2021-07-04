package com.lufinkey.soundholecore;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;

public class SoundHole {
	static {
		loadLibraries();
	}

	public static void loadLibraries() {
		com.lufinkey.DataCpp.loadLibraries();
		com.lufinkey.AsyncCpp.loadLibraries();
		com.lufinkey.IOCpp.loadLibraries();
		com.lufinkey.embed.NodeJS.loadLibraries();
		System.loadLibrary("SoundHoleCore");
	}

	public static void init(Context context) {
		com.lufinkey.soundholecore.android.Utils.appContext = context;
	}
}

package com.lufinkey.soundholecore;

public class NativeFunction {
	long func;

	public NativeFunction(long func) {
		this.func = func;
	}

	public void call(Object... args) {
		callFunction(func, args);
	}

	protected void finalize() throws Throwable {
		super.finalize();
		destroyFunction(func);
	}

	public native void callFunction(long func, Object[] arg);
	public native void destroyFunction(long func);
}

package com.lufinkey.soundholecore;

public class NativeCallback {
	NativeFunction onResolve;
	NativeFunction onReject;

	public NativeCallback(long onResolve, long onReject) {
		this.onResolve = new NativeFunction(onResolve);
		this.onReject = new NativeFunction(onReject);
	}

	public void resolve() {
		resolve(null);
	}

	public void resolve(Object result) {
		onResolve.call(result);
	}

	public void reject(Object error) {
		onReject.call(error);
	}
}

package com.lufinkey.soundholecore;

import com.spotify.sdk.android.player.*;

class NativeSpotifyPlayerOpCallback implements Player.OperationCallback {
	NativeCallback callback;

	public NativeSpotifyPlayerOpCallback(long onResolve, long onReject) {
		callback = new NativeCallback(onResolve, onReject);
	}

	@Override
	public void onSuccess() {
		callback.resolve();
	}

	@Override
	public void onError(com.spotify.sdk.android.player.Error error) {
		callback.resolve(error);
	}
}

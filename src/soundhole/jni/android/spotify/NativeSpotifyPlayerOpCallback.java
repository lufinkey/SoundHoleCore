package com.lufinkey.soundholecore.android.spotify;

import com.spotify.sdk.android.player.*;
import com.lufinkey.soundholecore.*;

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

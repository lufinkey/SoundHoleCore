package com.lufinkey.soundholecore;

import com.spotify.sdk.android.player.*;

public class NativeSpotifyPlayerInitCallback implements SpotifyPlayer.InitializationObserver {
	NativeCallback callback;

	public NativeSpotifyPlayerInitCallback(long onResolve, long onReject) {
		callback = new NativeCallback(onResolve, onReject);
	}

	@Override
	public void onInitialized(SpotifyPlayer player) {
		callback.resolve(player);
	}

	@Override
	public void onError(Throwable error) {
		callback.resolve(error);
	}
}

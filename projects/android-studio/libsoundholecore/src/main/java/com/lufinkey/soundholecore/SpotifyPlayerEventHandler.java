package com.lufinkey.soundholecore;

import com.spotify.sdk.android.player.*;

public class SpotifyPlayerEventHandler implements Player.NotificationCallback, ConnectionStateCallback {
	NativeFunction _onLoggedIn;
	NativeFunction _onLoggedOut;
	NativeFunction _onLoginFailed;
	NativeFunction _onTemporaryError;
	NativeFunction _onConnectionMessage;
	NativeFunction _onPlaybackEvent;
	NativeFunction _onPlaybackError;

	public SpotifyPlayerEventHandler(
			long onLoggedIn, long onLoggedOut, long onLoginFailed, long onTemporaryError, long onConnectionMessage,
			long onPlaybackEvent, long onPlaybackError) {
		_onLoggedIn = new NativeFunction(onLoggedIn);
		_onLoggedOut = new NativeFunction(onLoggedOut);
		_onLoginFailed = new NativeFunction(onLoginFailed);
		_onTemporaryError = new NativeFunction(onTemporaryError);
		_onConnectionMessage = new NativeFunction(onConnectionMessage);
		_onPlaybackEvent = new NativeFunction(onPlaybackEvent);
		_onPlaybackError = new NativeFunction(onPlaybackError);
	}

	@Override
	public void onLoggedIn() {
		_onLoggedIn.call();
	}

	@Override
	public void onLoggedOut() {
		_onLoggedOut.call();
	}

	@Override
	public void onLoginFailed(com.spotify.sdk.android.player.Error error) {
		_onLoginFailed.call(error);
	}

	@Override
	public void onTemporaryError() {
		_onTemporaryError.call();
	}

	@Override
	public void onConnectionMessage(String message) {
		_onConnectionMessage.call(message);
	}

	@Override
	public void onPlaybackEvent(PlayerEvent playerEvent) {
		_onPlaybackEvent.call(playerEvent);
	}

	@Override
	public void onPlaybackError(com.spotify.sdk.android.player.Error error) {
		_onPlaybackError.call(error);
	}
}

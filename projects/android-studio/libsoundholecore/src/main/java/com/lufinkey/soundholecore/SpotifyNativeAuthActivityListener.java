package com.lufinkey.soundholecore;

class SpotifyNativeAuthActivityListener implements SpotifyAuthActivityListener {
	private NativeFunction _onSpotifyAuthReceiveSession;
	private NativeFunction _onSpotifyAuthReceiveCode;
	private NativeFunction _onSpotifyAuthCancel;
	private NativeFunction _onSpotifyAuthFailure;

	SpotifyNativeAuthActivityListener(
		long onSpotifyAuthReceiveSession, long onSpotifyAuthReceiveCode,
		long onSpotifyAuthCancel, long onSpotifyAuthFailure) {
		_onSpotifyAuthReceiveSession = new NativeFunction(onSpotifyAuthReceiveSession);
		_onSpotifyAuthReceiveCode = new NativeFunction(onSpotifyAuthReceiveCode);
		_onSpotifyAuthCancel = new NativeFunction(onSpotifyAuthCancel);
		_onSpotifyAuthFailure = new NativeFunction(onSpotifyAuthFailure);
	}

	public void onSpotifyAuthReceiveSession(SpotifyAuthActivity activity, SpotifySession session) {
		_onSpotifyAuthReceiveSession.call(activity, session);
	}

	public void onSpotifyAuthReceiveCode(SpotifyAuthActivity activity, String code) {
		_onSpotifyAuthReceiveCode.call(activity, code);
	}

	public void onSpotifyAuthCancel(SpotifyAuthActivity activity) {
		_onSpotifyAuthCancel.call(activity);
	}

	public void onSpotifyAuthFailure(SpotifyAuthActivity activity, String error) {
		_onSpotifyAuthFailure.call(activity, error);
	}
}

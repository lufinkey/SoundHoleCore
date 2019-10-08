package com.lufinkey.soundholecore;

public interface SpotifyAuthActivityListener {
	void onSpotifyAuthCancel(SpotifyAuthActivity activity);
	void onSpotifyAuthFailure(SpotifyAuthActivity activity, String error);
	void onSpotifyAuthReceiveSession(SpotifyAuthActivity activity, SpotifySession session);
	void onSpotifyAuthReceiveCode(SpotifyAuthActivity activity, String code);
}

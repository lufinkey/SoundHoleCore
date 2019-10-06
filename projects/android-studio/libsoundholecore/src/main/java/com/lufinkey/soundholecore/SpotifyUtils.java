package com.lufinkey.soundholecore;

import android.content.Context;

import com.spotify.sdk.android.player.*;

public class SpotifyUtils {
	static {
		initPlayerUtils(
			com.spotify.sdk.android.player.SpotifyPlayer.class,
			com.spotify.sdk.android.player.PlaybackState.class,
			com.spotify.sdk.android.player.Metadata.class,
			com.spotify.sdk.android.player.Metadata.Track.class);
		initErrorUtils(com.spotify.sdk.android.player.Error.class);
	}

	private static native void initPlayerUtils(Class spotifyPlayerClass, Class stateClass, Class metadataClass, Class trackClass);
	private static native void initErrorUtils(Class errorClass);
	public static Context appContext;

	public void getPlayer(String clientId, String accessToken, final NativeSpotifyPlayerInitCallback callback) {
		final Object reference = this;
		Config config = new Config(appContext, accessToken, clientId);
		Spotify.getPlayer(config, reference, callback);
	}

	public void destroyPlayer(SpotifyPlayer player) {
		if(player != null) {
			player.destroy();
		}
		Spotify.destroyPlayer(this);
	}
}

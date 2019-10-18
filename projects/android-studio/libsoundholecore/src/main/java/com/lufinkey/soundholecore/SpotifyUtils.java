package com.lufinkey.soundholecore;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

import com.spotify.sdk.android.player.*;

class SpotifyUtils {
	public void getPlayer(String clientId, String accessToken, final NativeSpotifyPlayerInitCallback callback) {
		final Object reference = this;
		Config config = new Config(Utils.appContext, accessToken, clientId);
		Spotify.getPlayer(config, reference, callback);
	}

	public void destroyPlayer(SpotifyPlayer player) {
		if(player != null) {
			player.destroy();
		}
		Spotify.destroyPlayer(this);
	}

	public static Connectivity getNetworkConnectivity() {
		assert Utils.appContext != null : "SoundHole has not been initialized";
		ConnectivityManager connectivityManager = (ConnectivityManager)Utils.appContext.getSystemService(Context.CONNECTIVITY_SERVICE);
		NetworkInfo activeNetwork = connectivityManager.getActiveNetworkInfo();
		if (activeNetwork != null && activeNetwork.isConnected()) {
			return Connectivity.fromNetworkType(activeNetwork.getType());
		}
		else {
			return Connectivity.OFFLINE;
		}
	}
}
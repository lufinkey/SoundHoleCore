package com.lufinkey.soundholecore.android.spotify;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

import com.spotify.sdk.android.player.*;

import com.lufinkey.soundholecore.*;
import com.lufinkey.soundholecore.android.*;

class SpotifyUtils {
	public void getPlayer(String clientId, String accessToken, final NativeSpotifyPlayerInitCallback callback) {
		final Object reference = this;
		Config config = new Config(Utils.appContext, accessToken, clientId);
		Spotify.getPlayer(config, reference, callback);
	}

	public void destroyPlayer(SpotifyPlayer player) {
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

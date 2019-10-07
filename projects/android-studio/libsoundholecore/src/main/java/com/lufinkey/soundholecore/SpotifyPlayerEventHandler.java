package com.lufinkey.soundholecore;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;

import com.spotify.sdk.android.player.*;

public class SpotifyPlayerEventHandler implements Player.NotificationCallback, ConnectionStateCallback {
	private SpotifyPlayer player;
	NativeFunction _onLoggedIn;
	NativeFunction _onLoggedOut;
	NativeFunction _onLoginFailed;
	NativeFunction _onTemporaryError;
	NativeFunction _onConnectionMessage;
	NativeFunction _onDisconnect;
	NativeFunction _onReconnect;
	NativeFunction _onPlaybackEvent;
	NativeFunction _onPlaybackError;
	private BroadcastReceiver networkStateReceiver;
	private Connectivity currentConnectivity = Connectivity.OFFLINE;

	public SpotifyPlayerEventHandler(
			SpotifyPlayer player,
			long onLoggedIn, long onLoggedOut, long onLoginFailed, long onTemporaryError, long onConnectionMessage,
			long onDisconnect, long onReconnect,
			long onPlaybackEvent, long onPlaybackError) {
		this.player = player;
		_onLoggedIn = new NativeFunction(onLoggedIn);
		_onLoggedOut = new NativeFunction(onLoggedOut);
		_onLoginFailed = new NativeFunction(onLoginFailed);
		_onTemporaryError = new NativeFunction(onTemporaryError);
		_onConnectionMessage = new NativeFunction(onConnectionMessage);
		_onDisconnect = new NativeFunction(onDisconnect);
		_onReconnect = new NativeFunction(onReconnect);
		_onPlaybackEvent = new NativeFunction(onPlaybackEvent);
		_onPlaybackError = new NativeFunction(onPlaybackError);

		currentConnectivity = SpotifyUtils.getNetworkConnectivity();
		final SpotifyPlayerEventHandler _this = this;
		networkStateReceiver = new BroadcastReceiver() {
			@Override
			public void onReceive(Context context, Intent intent) {
				Connectivity prevConnectivity = currentConnectivity;
				Connectivity connectivity = SpotifyUtils.getNetworkConnectivity();
				currentConnectivity = connectivity;
				if (_this.player != null && _this.player.isInitialized()) {
					// update the player with the connection state, because Android makes no sense
					_this.player.setConnectivityStatus(null, connectivity);
				}

				if(prevConnectivity==Connectivity.OFFLINE && connectivity!=Connectivity.OFFLINE) {
					_onReconnect.call();
				}
				else if(prevConnectivity!=Connectivity.OFFLINE && connectivity==Connectivity.OFFLINE) {
					_onDisconnect.call();
				}
			}
		};
		IntentFilter filter = new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION);
		SoundHole.getMainContext().registerReceiver(networkStateReceiver, filter);

		player.setConnectivityStatus(null, currentConnectivity);
		player.addNotificationCallback(this);
		player.addConnectionStateCallback(this);
	}

	void destroy() {
		SoundHole.getMainContext().unregisterReceiver(networkStateReceiver);
		player.removeNotificationCallback(this);
		player.removeConnectionStateCallback(this);
		player = null;
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

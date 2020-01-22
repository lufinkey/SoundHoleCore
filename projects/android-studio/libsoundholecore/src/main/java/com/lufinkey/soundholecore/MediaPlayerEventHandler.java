package com.lufinkey.soundholecore;

import android.media.MediaPlayer;

public class MediaPlayerEventHandler implements MediaPlayer.OnCompletionListener, MediaPlayer.OnErrorListener,
		MediaPlayer.OnInfoListener, MediaPlayer.OnPreparedListener, MediaPlayer.OnSeekCompleteListener {

	NativeFunction _onCompletion;
	NativeFunction _onError;
	NativeFunction _onInfo;
	NativeFunction _onPrepared;
	NativeFunction _onSeekComplete;

	public MediaPlayerEventHandler(
			long onCompletion, long onError, long onInfo, long onPrepared, long onSeekComplete) {
		this._onCompletion = new NativeFunction(onCompletion);
		this._onError = new NativeFunction(onError);
		this._onInfo = new NativeFunction(onInfo);
		this._onPrepared = new NativeFunction(onPrepared);
		this._onSeekComplete = new NativeFunction(onSeekComplete);
	}

	void attach(MediaPlayer player) {
		player.setOnCompletionListener(this);
		player.setOnErrorListener(this);
		player.setOnInfoListener(this);
		player.setOnPreparedListener(this);
		player.setOnSeekCompleteListener(this);
	}

	void detach(MediaPlayer player) {
		player.setOnCompletionListener(null);
		player.setOnErrorListener(null);
		player.setOnInfoListener(null);
		player.setOnPreparedListener(null);
		player.setOnSeekCompleteListener(null);
	}

	@Override
	public void onCompletion(MediaPlayer mp) {
		//
	}

	@Override
	public boolean onError(MediaPlayer mp, int what, int extra) {
		return false;
	}

	@Override
	public boolean onInfo(MediaPlayer mp, int what, int extra) {
		return false;
	}

	@Override
	public void onPrepared(MediaPlayer mp) {
		//
	}

	@Override
	public void onSeekComplete(MediaPlayer mp) {
		//
	}
}

package com.lufinkey.soundholecore;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;

import com.spotify.sdk.android.authentication.AuthenticationClient;
import com.spotify.sdk.android.authentication.AuthenticationRequest;
import com.spotify.sdk.android.authentication.AuthenticationResponse;

import java.util.UUID;

public class SpotifyAuthActivity extends Activity {
	private static final int REQUEST_CODE = 42069;

	private static SpotifyLoginOptions authFlow_options;
	private static SpotifyAuthActivityListener authFlow_listener;
	private static SpotifyAuthActivity currentAuthActivity;

	private SpotifyLoginOptions options;
	private String xssState;
	private SpotifyAuthActivityListener listener;
	private NativeFunction finishCompletion;

	public static void performAuthFlow(Context context, SpotifyLoginOptions options, SpotifyAuthActivityListener listener) {
		// ensure no conflicting callbacks
		if(authFlow_options != null || authFlow_listener != null || currentAuthActivity != null) {
			listener.onSpotifyAuthFailure(null, "Cannot call login or authenticate multiple times before completing");
			return;
		}

		// store temporary static variables
		authFlow_options = options;
		authFlow_listener = listener;

		// start activity
		Intent intent = new Intent(context, SpotifyAuthActivity.class);
		intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		context.startActivity(intent);
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setFinishOnTouchOutside(false);

		options = authFlow_options;
		xssState = UUID.randomUUID().toString();
		listener = authFlow_listener;
		currentAuthActivity = this;

		authFlow_options = null;
		authFlow_listener = null;

		AuthenticationRequest request = options.getAuthenticationRequest(xssState);
		AuthenticationClient.openLoginActivity(this, REQUEST_CODE, request);
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent intent) {
		super.onActivityResult(requestCode, resultCode, intent);

		if(requestCode == REQUEST_CODE) {
			AuthenticationResponse response = AuthenticationClient.getResponse(resultCode, intent);

			switch(response.getType()) {
				default:
					listener.onSpotifyAuthCancel(this);
					break;

				case ERROR:
					if(response.getError().equals("access_denied")) {
						listener.onSpotifyAuthCancel(this);
					}
					else {
						listener.onSpotifyAuthFailure(this, response.getError());
					}
					break;

				case TOKEN:
					/*if(xssState != null && !xssState.equals(response.getState())) {
						listener.onAuthActivityFailure(this, new SpotifyError("state_mismatch", "state mismatch"));
						return;
					}*/
					SpotifySession sessionData = new SpotifySession();
					sessionData.accessToken = response.getAccessToken();
					sessionData.expireTime = SpotifySession.getExpireDate(response.getExpiresIn()).getTime();
					sessionData.refreshToken = null;
					sessionData.scopes = options.scopes;
					listener.onSpotifyAuthReceiveSession(this, sessionData);
					break;

				case CODE:
					/*if(xssState != null && !xssState.equals(response.getState())) {
						listener.onAuthActivityFailure(this, new SpotifyError("state_mismatch", "state mismatch"));
						return;
					}*/
					listener.onSpotifyAuthReceiveCode(this, response.getCode());
					break;
			}
		}
	}

	public void finish(NativeFunction completion) {
		finishCompletion = completion;
		this.finish();
	}

	@Override
	public void finish() {
		AuthenticationClient.stopLoginActivity(this, REQUEST_CODE);
		super.finish();
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		if(isFinishing()) {
			currentAuthActivity = null;
			if(finishCompletion != null) {
				NativeFunction completion = finishCompletion;
				finishCompletion = null;
				completion.call();
			}
		}
	}
}

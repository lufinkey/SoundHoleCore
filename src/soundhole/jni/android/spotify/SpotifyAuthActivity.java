package com.lufinkey.soundholecore.android.spotify;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.view.Gravity;
import android.widget.ProgressBar;

import com.spotify.sdk.android.authentication.AuthenticationClient;
import com.spotify.sdk.android.authentication.AuthenticationRequest;
import com.spotify.sdk.android.authentication.AuthenticationResponse;

import java.util.UUID;

import com.lufinkey.soundholecore.*;

public class SpotifyAuthActivity extends Activity {
	private static final int REQUEST_CODE = 42069;

	private static SpotifyAuthenticateOptions authFlow_options;
	private static SpotifyAuthActivityListener authFlow_listener;
	private static SpotifyAuthActivity currentAuthActivity;

	private SpotifyAuthenticateOptions options;
	private String xssState;
	private SpotifyAuthActivityListener listener;
	private NativeFunction finishCompletion;

	private ProgressDialog progressDialog;

	public static void performAuthFlow(Context context, SpotifyAuthenticateOptions options, SpotifyAuthActivityListener listener) {
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
					} else {
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
					// TODO set sessionData.tokenType
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


	private void showProgressDialog(String loadingText) {
		if(progressDialog != null) {
			progressDialog.setMessage(loadingText);
			return;
		}
		ProgressDialog dialog = new ProgressDialog(this, android.R.style.Theme_Black);
		dialog.getWindow().setBackgroundDrawableResource(android.R.color.transparent);
		dialog.getWindow().setGravity(Gravity.CENTER);
		if(loadingText != null) {
			dialog.setMessage(loadingText);
		}
		dialog.setCancelable(false);
		dialog.setIndeterminate(true);
		dialog.show();
		progressDialog = dialog;
	}

	private void hideProgressDialog() {
		if(progressDialog != null) {
			progressDialog.dismiss();
			progressDialog = null;
		}
	}
}

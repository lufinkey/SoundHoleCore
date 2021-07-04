package com.lufinkey.soundholecore.android.spotify;

import com.spotify.sdk.android.authentication.*;

import java.util.HashMap;

public class SpotifyAuthenticateOptions {
	public String clientId = null;
	public String clientSecret = null;
	public String redirectURL = null;
	public String[] scopes = null;
	public String tokenSwapURL = null;
	public Boolean showDialog = null;
	public String loginLoadingText = null;

	AuthenticationRequest getAuthenticationRequest(String state) {
		AuthenticationResponse.Type responseType = AuthenticationResponse.Type.TOKEN;
		if(tokenSwapURL != null) {
			responseType = AuthenticationResponse.Type.CODE;
		}
		AuthenticationRequest.Builder requestBuilder = new AuthenticationRequest.Builder(clientId, responseType, redirectURL);
		if(scopes != null) {
			requestBuilder.setScopes(scopes);
		}
		/*if(state != null) {
			requestBuilder.setState(state);
		}*/
		if(showDialog != null) {
			requestBuilder.setShowDialog(showDialog);
		}
		return requestBuilder.build();
	}
}

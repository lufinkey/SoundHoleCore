package com.lufinkey.soundholecore;

import com.spotify.sdk.android.authentication.*;

import java.util.HashMap;

public class SpotifyLoginOptions {
	public String clientId = null;
	public String redirectURL = null;
	public String[] scopes = null;
	public String tokenSwapURL = null;
	public String tokenRefreshURL = null;
	public HashMap<String,String> params = null;

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
		if(params != null) {
			String showDialog = params.get("show_dialog");
			if(showDialog != null) {
				requestBuilder.setShowDialog(showDialog.equals("true") ? true : false);
			}
		}
		return requestBuilder.build();
	}
}

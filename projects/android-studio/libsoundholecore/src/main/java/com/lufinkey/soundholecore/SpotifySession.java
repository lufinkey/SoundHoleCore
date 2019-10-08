package com.lufinkey.soundholecore;

import java.util.Calendar;
import java.util.Date;

public class SpotifySession {
	public String accessToken = null;
	public long expireTime = 0;
	public String refreshToken = null;
	public String[] scopes = null;

	public boolean isValid() {
		if(accessToken == null || expireTime == 0 || expireTime < (new Date()).getTime()) {
			return false;
		}
		return true;
	}

	public boolean hasScope(String scope) {
		if(scopes == null) {
			return false;
		}
		for(String cmpScope : scopes) {
			if(scope.equals(cmpScope)) {
				return true;
			}
		}
		return false;
	}

	public static Date getExpireDate(int expireSeconds) {
		Calendar calendar = Calendar.getInstance();
		calendar.setTime(new Date());
		calendar.add(Calendar.SECOND, expireSeconds);
		return calendar.getTime();
	}
}

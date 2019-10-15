//
//  SoundHoleCoreTest.cpp
//  SoundHoleCoreTest
//
//  Created by Luis Finke on 9/1/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SoundHoleCoreTest.hpp"
#include <soundhole/providers/spotify/api/Spotify.hpp>

namespace sh::test {
	Promise<void> testSpotify() {
		printf("beginning spotify tests\n");

		auto spotify = new Spotify({
			.auth = {
				.clientId = "b2f54a1af8c943eeabbf2b197c53d173",
				.redirectURL = "soundhole://spotify-auth",
				.scopes = {"user-library-read", "user-read-private", "playlist-read", "playlist-read-private", "streaming"},
				.tokenSwapURL = "http://spotifytokenrefresh.iwouldtotallyfuck.me:5080/swap.php",
				.tokenRefreshURL = "http://spotifytokenrefresh.iwouldtotallyfuck.me:5080/refresh.php",
				.sessionPersistKey = "SpotifySession"
			}
		});
		
		auto promise = Promise<void>::resolve();
		
		promise = promise.then([=]() -> Promise<void> {
			printf("logging out spotify\n");
			return spotify->logout();
		});

		promise = promise.then([=]() -> Promise<void> {
			if(!spotify->isLoggedIn()) {
				return spotify->login().then([=](bool loggedIn) {
					if(loggedIn) {
						printf("login succeeded\n");
					} else {
						printf("login cancelled\n");
					}
				}).except([=](std::exception_ptr errorPtr) {
					try {
						std::rethrow_exception(errorPtr);
					} catch(SpotifyError& error) {
						printf("login failed with error: %s\n", error.toString().c_str());
					} catch(Error& error) {
						printf("login failed with error: %s\n", error.toString().c_str());
					} catch(std::exception& error) {
						printf("login failed with exception: %s\n", error.what());
					} catch(...) {
						printf("login failed with unknown error\n");
					}
					std::rethrow_exception(errorPtr);
				});
			} else {
				printf("spotify is already logged in\n");
				return Promise<void>::resolve();
			}
		});
		
		promise = promise.then([=]() -> Promise<void> {
			printf("playing song\n");
			return spotify->playURI("spotify:track:1ONC00cKNdFgKiATMtcxEc").then([=]() -> Promise<void> {
				printf("delaying for 10 seconds\n");
				return Timer::delay(std::chrono::seconds(10));
			}).except([=](std::exception_ptr errorPtr) {
				try {
					std::rethrow_exception(errorPtr);
				} catch(SpotifyError& error) {
					printf("playURI failed with error: %s\n", error.toString().c_str());
				} catch(Error& error) {
					printf("playURI failed with error: %s\n", error.toString().c_str());
				} catch(std::exception& error) {
					printf("playURI failed with exception: %s\n", error.what());
				} catch(...) {
					printf("playURI failed with unknown error\n");
				}
				std::rethrow_exception(errorPtr);
			});
		});
		
		promise = promise.finally([=]() {
			printf("cleaning up spotify instance\n");
			delete spotify;
		});
		
		return promise;
	}
}

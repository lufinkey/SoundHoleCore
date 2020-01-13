//
//  SoundHoleCoreTest.cpp
//  SoundHoleCoreTest
//
//  Created by Luis Finke on 9/1/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SoundHoleCoreTest.hpp"
#include <soundhole/providers/spotify/api/Spotify.hpp>
#include <soundhole/playback/StreamPlayer.hpp>

namespace sh::test {
	Promise<void> testSpotify() {
		printf("testing Spotify\n");

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
		
		return Promise<void>::resolve()
		.then([=]() {
			printf("logging out spotify\n");
			return spotify->logout();
		})
		.then([=]() {
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
		})
		.then([=]() {
			printf("playing song\n");
			return spotify->playURI("spotify:track:1ONC00cKNdFgKiATMtcxEc")
			.delay(std::chrono::seconds(10))
			.except([=](std::exception_ptr errorPtr) {
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
		})
		.then([=]() {
			printf("stopping spotify\n");
			spotify->stopPlayer();
		})
		.finally([=]() {
			printf("cleaning up spotify instance\n");
			delete spotify;
			printf("\n");
		});
	}
	
	
	
	Promise<void> testStreamPlayer() {
		printf("testing StreamPlayer\n");
		
		auto streamPlayer = new StreamPlayer();
		
		String iWishIWasAChair = "https://t4.bcbits.com/stream/eaf3df4fb945d596ecec739ce4dea428/mp3-128/1350603412?p=0&ts=1578972318&t=5629ea641c9f18a90485763182a417e170e9c17c&token=1578972318_703260b281c45e78e9ee8e3fdf02412738fb236d";
		
		String endOfTheNight = "https://t4.bcbits.com/stream/9cec9cf3b8981b020112f369dd36b7ff/mp3-128/2708171616?p=0&ts=1578973443&t=7639636dfb9feac2d56d4d7ada9beb133d34ebbd&token=1578973443_7278cdd10a35fca4098b288295723d1e3f8992c8";
		
		return Promise<void>::resolve()
		.then([=]() {
			printf("playing: I Wish I was a Chair.\n");
			return streamPlayer->play(iWishIWasAChair);
		})
		.delay(std::chrono::seconds(5))
		.then([=]() {
			printf("preparing: End Of The niGht. (AF THE NAYSAYER Remix)\n");
			return streamPlayer->prepare(endOfTheNight);
		})
		.delay(std::chrono::seconds(5))
		.then([=]() {
			printf("playing: End Of The niGht. (AF THE NAYSAYER Remix)\n");
			return streamPlayer->play(endOfTheNight);
		})
		.delay(std::chrono::seconds(10))
		.finally([=]() {
			printf("stopping\n");
			return streamPlayer->stop();
		})
		.finally([=]() {
			printf("deleting StreamPlayer\n");
			delete streamPlayer;
			printf("\n");
		});
	}
}

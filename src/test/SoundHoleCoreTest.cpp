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
#ifdef __ANDROID__
#include <android/log.h>
#endif

#ifdef __ANDROID__
	#define PRINT(...) __android_log_print(ANDROID_LOG_DEBUG, "SoundHoleCoreTest", __VA_ARGS__)
#else
	#define PRINT(...) printf(__VA_ARGS__)
#endif

namespace sh::test {
	Promise<void> testSpotify() {
		PRINT("testing Spotify\n");

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
			PRINT("logging out spotify\n");
			return spotify->logout();
		})
		.then([=]() {
			if(!spotify->isLoggedIn()) {
				return spotify->login().then([=](bool loggedIn) {
					if(loggedIn) {
						PRINT("login succeeded\n");
					} else {
						PRINT("login cancelled\n");
					}
				}).except([=](std::exception_ptr errorPtr) {
					try {
						std::rethrow_exception(errorPtr);
					} catch(SpotifyError& error) {
						PRINT("login failed with error: %s\n", error.toString().c_str());
					} catch(Error& error) {
						PRINT("login failed with error: %s\n", error.toString().c_str());
					} catch(std::exception& error) {
						PRINT("login failed with exception: %s\n", error.what());
					} catch(...) {
						PRINT("login failed with unknown error\n");
					}
					std::rethrow_exception(errorPtr);
				});
			} else {
				PRINT("spotify is already logged in\n");
				return Promise<void>::resolve();
			}
		})
		.then([=]() {
			PRINT("playing song\n");
			return spotify->playURI("spotify:track:1aVedqqfdBKK0XsrjNJerA")
			.delay(std::chrono::seconds(10))
			.except([=](std::exception_ptr errorPtr) {
				try {
					std::rethrow_exception(errorPtr);
				} catch(SpotifyError& error) {
					PRINT("playURI failed with error: %s\n", error.toString().c_str());
				} catch(Error& error) {
					PRINT("playURI failed with error: %s\n", error.toString().c_str());
				} catch(std::exception& error) {
					PRINT("playURI failed with exception: %s\n", error.what());
				} catch(...) {
					PRINT("playURI failed with unknown error\n");
				}
				std::rethrow_exception(errorPtr);
			});
		})
		.then([=]() {
			PRINT("stopping spotify\n");
			spotify->stopPlayer();
		})
		.finally([=]() {
			PRINT("cleaning up spotify instance\n");
			delete spotify;
			PRINT("\n");
		});
	}
	
	
	
	Promise<void> testStreamPlayer() {
		PRINT("testing StreamPlayer\n");
		
		auto streamPlayer = new StreamPlayer();
		auto bandcamp = new Bandcamp();
		
		struct SharedData {
			Optional<BandcampTrack> firstTrack;
			Optional<BandcampTrack> secondTrack;
		};
		$<SharedData> sharedData = new$<SharedData>();
		
		return Promise<void>::resolve()
		.then([=]() {
			PRINT("fetching Phonyland album\n");
			return bandcamp->getAlbum("https://phonyland.bandcamp.com/album/phonyland").then([=](BandcampAlbum album) {
				PRINT("applying Phonyland album\n");
				if(!album.tracks || album.tracks->size() == 0) {
					PRINT("no tracks\n");
					throw std::runtime_error("Phonyland album has no tracks");
				}
				auto tracks = album.tracks->where([](auto& track) {
					return (track.audioSources && track.audioSources->size() > 0);
				});
				if(tracks.size() == 0) {
					PRINT("no playable tracks\n");
					throw std::runtime_error("Phonyland album has no playable tracks");
				}
				sharedData->firstTrack = tracks.at((size_t)(tracks.size() * ((double)rand()/(double)RAND_MAX)));
				sharedData->secondTrack = tracks.at((size_t)(tracks.size() * ((double)rand()/(double)RAND_MAX)));
			});
		})
		.then([=]() {
			PRINT("playing: %s\n", sharedData->firstTrack->name.c_str());
			return streamPlayer->play(sharedData->firstTrack->audioSources->front().url);
		})
		.delay(std::chrono::seconds(5))
		.then([=]() {
			PRINT("preparing: %s\n", sharedData->secondTrack->name.c_str());
			return streamPlayer->prepare(sharedData->secondTrack->audioSources->front().url);
		})
		.delay(std::chrono::seconds(5))
		.then([=]() {
			PRINT("playing: %s\n", sharedData->secondTrack->name.c_str());
			return streamPlayer->play(sharedData->secondTrack->audioSources->front().url);
		})
		.delay(std::chrono::seconds(10))
		.finally([=]() {
			PRINT("stopping StreamPlayer\n");
			return streamPlayer->stop();
		})
		.finally([=]() {
			PRINT("deleting StreamPlayer\n");
			delete streamPlayer;
			delete bandcamp;
			PRINT("\n");
		});
	}
}

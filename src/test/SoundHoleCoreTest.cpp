//
//  SoundHoleCoreTest.cpp
//  SoundHoleCoreTest
//
//  Created by Luis Finke on 9/1/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "SoundHoleCoreTest.hpp"
#include <soundhole/providers/spotify/api/Spotify.hpp>
#include <soundhole/providers/bandcamp/api/Bandcamp.hpp>
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
	Promise<void> runTests() {
		return Promise<void>::resolve()
		.then([=]() {
			return testBandcamp();
		})
		.then([=]() {
			return testSpotify();
		})
		.then([=]() {
			return sh::test::testStreamPlayer();
		})
		.except([=](std::exception_ptr error) {
			fgl::console::error("error: ", sh::utils::getExceptionDetails(error).fullDescription);
		});
	}



	Promise<void> testBandcamp() {
		auto bandcamp = new Bandcamp({
			.auth = {
				.sessionPersistKey = "BandcampSession"
			}
		});
		
		return Promise<void>::resolve()
		.then([=]() {
			PRINT("logging out bandcamp\n");
			return bandcamp->logout();
		})
		.then([=]() {
			if(bandcamp->isLoggedIn()) {
				PRINT("bandcamp is already logged in\n");
				return Promise<void>::resolve();
			}
			return bandcamp->login().then([=](bool loggedIn) {
				if(loggedIn) {
					PRINT("login succeeded\n");
				} else {
					PRINT("login cancelled\n");
				}
			}).except([=](std::exception_ptr error) {
				try {
					std::rethrow_exception(error);
				} catch(...) {
					PRINT("login failed with error: %s\n", sh::utils::getExceptionDetails(error).fullDescription.c_str());
					std::rethrow_exception(std::current_exception());
				}
			});
		})
		.delay(std::chrono::seconds(5))
		.finally([=]() {
			PRINT("cleaning up spotify instance\n");
			delete bandcamp;
			PRINT("\n");
		});
	}



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
			if(spotify->isLoggedIn()) {
				PRINT("spotify is already logged in\n");
				return Promise<void>::resolve();
			}
			return spotify->login().then([=](bool loggedIn) {
				if(loggedIn) {
					PRINT("login succeeded\n");
				} else {
					PRINT("login cancelled\n");
				}
			}).except([=](std::exception_ptr error) {
				try {
					std::rethrow_exception(error);
				} catch(...) {
					PRINT("login failed with error: %s\n", sh::utils::getExceptionDetails(error).fullDescription.c_str());
					std::rethrow_exception(std::current_exception());
				}
			});
		})
		.then([=]() {
			return spotify->getMyTracks({
				.market="from_token"
			}).then([=](SpotifyPage<SpotifySavedTrack> tracks) {
				printf("fetched %i tracks", (int)tracks.items.size());
			});
		})
		.then([=]() {
			PRINT("playing song\n");
			return spotify->playURI("spotify:track:1aVedqqfdBKK0XsrjNJerA")
			.delay(std::chrono::seconds(10))
			.except([=](std::exception_ptr error) {
				try {
					std::rethrow_exception(error);
				} catch(...) {
					PRINT("playURI failed with error: %s\n", sh::utils::getExceptionDetails(error).fullDescription.c_str());
					std::rethrow_exception(std::current_exception());
				}
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
		auto bandcamp = new Bandcamp({});
		
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

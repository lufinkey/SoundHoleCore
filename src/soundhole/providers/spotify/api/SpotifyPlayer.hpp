//
//  SpotifyPlayer.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 9/22/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <mutex>
#include <soundhole/common.hpp>
#ifdef __OBJC__
#import <Foundation/Foundation.h>
#endif
#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
#import <SpotifyAudioPlayback/SpotifyAudioPlayback.h>
#include "SpotifyPlayerEventHandler_iOS.hpp"
#endif

#include "SpotifyAuth.hpp"
#include "SpotifyError.hpp"
#include "SpotifyAuthEventListener.hpp"

namespace sh {
	class SpotifyPlayer: protected SpotifyAuthEventListener {
	public:
		static SpotifyPlayer* const shared;
		
		struct State {
			bool playing;
			bool repeating;
			bool shuffling;
			bool activeDevice;
			double position;
		};
		
		struct Track {
			String uri;
			String name;
			String contextURI;
			String contextName;
			String artistURI;
			String artistName;
			String albumURI;
			String albumName;
			Optional<String> albumCoverArtURL;
			double duration;
			size_t indexInContext;
		};
		
		struct Metadata {
			Optional<Track> previousTrack;
			Optional<Track> currentTrack;
			Optional<Track> nextTrack;
		};
		
		void setAuth(SpotifyAuth* auth);
		SpotifyAuth* getAuth();
		const SpotifyAuth* getAuth() const;
		
		Promise<void> start();
		void stop();
		bool isStarted() const;
		
		Promise<void> login();
		Promise<void> logout();
		bool isLoggedIn() const;
		
		struct PlayOptions {
			size_t index = 0;
			double position = 0;
		};
		Promise<void> playURI(String uri, PlayOptions options = {.index=0,.position=0});
		Promise<void> queueURI(String uri);
		
		Promise<void> skipToNext();
		Promise<void> skipToPrevious();
		Promise<void> seek(double position);
		
		Promise<void> setPlaying(bool playing);
		Promise<void> setShuffling(bool shuffling);
		Promise<void> setRepeating(bool repeating);
		
		State getState() const;
		Metadata getMetadata() const;
		
		#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
		static State stateFromSPTPlaybackState(SPTPlaybackState* state);
		static Track trackFromSPTPlaybackTrack(SPTPlaybackTrack* track);
		static Metadata metadataFromSPTPlaybackMetadata(SPTPlaybackMetadata* metadata);
		#endif
		
	protected:
		virtual void onSpotifyAuthSessionResume(SpotifyAuth* auth) override;
		virtual void onSpotifyAuthSessionStart(SpotifyAuth* auth) override;
		virtual void onSpotifyAuthSessionRenew(SpotifyAuth* auth) override;
		virtual void onSpotifyAuthSessionEnd(SpotifyAuth* auth) override;
		
	private:
		SpotifyPlayer();
		
		struct WaitCallback {
			typename Promise<void>::Resolver resolve;
			typename Promise<void>::Rejecter reject;
		};
		
		struct PrepareForCallOptions {
			bool start = true;
			bool login = true;
		};
		Promise<void> prepareForCall(bool condition = true);
		Promise<bool> startIfAble();
		Promise<bool> loginIfAble();
		bool isPlayerLoggedIn() const;
		
		void applyAuthToken(String accessToken);
		
		void beginSession();
		void renewSession(String accessToken);
		void endSession();
		
		void onStreamingLogin();
		void onStreamingLogout();
		void onStreamingError(SpotifyError);
		
		SpotifyAuth* auth;
		#ifdef TARGETPLATFORM_IOS
		OBJCPP_PTR(SPTAudioStreamingController) player;
		OBJCPP_PTR(SpotifyPlayerEventHandler) playerEventHandler;
		#endif
		
		bool starting;
		LinkedList<WaitCallback> startCallbacks;
		std::mutex startMutex;
		
		bool loggingIn;
		bool loggedIn;
		bool loggingOut;
		bool renewingSession;
		LinkedList<WaitCallback> loginCallbacks;
		LinkedList<WaitCallback> logoutCallbacks;
		std::mutex loginMutex;
		
		#ifdef TARGETPLATFORM_IOS
		struct SavedPlayerState {
			String uri;
			size_t index;
			double position;
			bool shuffling;
			bool repeating;
			bool playing;
		};
		void restoreSavedPlayerState(const SavedPlayerState& state);
		SavedPlayerState getPlayerState();
		Optional<SavedPlayerState> renewingPlayerState;
		#endif
	};
}

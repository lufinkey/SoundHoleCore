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
#include "SpotifyPlayerEventListener.hpp"
#if defined(JNIEXPORT) && defined(__ANDROID__)
#include <soundhole/jnicpp/android/spotify/SpotifyUtils_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyPlayer_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyPlayerEventHandler_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyTrack_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyPlaybackState_jni.hpp>
#include <soundhole/jnicpp/android/spotify/SpotifyMetadata_jni.hpp>
#endif

namespace sh {
	class SpotifyPlayer: protected SpotifyAuthEventListener {
	public:
		static SpotifyPlayer* shared();
		
		struct Options {
			struct iOS {
				String audioSessionCategory;
			} ios;
		};
		
		struct State {
			bool playing = false;
			bool repeating = false;
			bool shuffling = false;
			bool activeDevice = false;
			double position = 0.0;
		};
		
		struct Track {
			String uri;
			String name;
			String artistURI;
			String artistName;
			String albumURI;
			String albumName;
			Optional<String> albumCoverArtURL;
			double duration = 0.0;

			Optional<size_t> indexInContext;
			Optional<String> contextURI;
            Optional<String> contextName;
		};
		
		struct Metadata {
			Optional<Track> previousTrack;
			Optional<Track> currentTrack;
			Optional<Track> nextTrack;
		};

		~SpotifyPlayer();
		
		void addEventListener(SpotifyPlayerEventListener* listener);
		void removeEventListener(SpotifyPlayerEventListener* listener);
		
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
		
		void setOptions(Options options);
		const Options& getOptions() const;
		
		State getState() const;
		Metadata getMetadata() const;
		
		#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
		static State stateFromSPTPlaybackState(SPTPlaybackState* state);
		static Track trackFromSPTPlaybackTrack(SPTPlaybackTrack* track);
		static Metadata metadataFromSPTPlaybackMetadata(SPTPlaybackMetadata* metadata);
		#endif
		#if defined(JNIEXPORT) && defined(__ANDROID__)
		static State stateFromAndroidState(JNIEnv* env, jni::android::spotify::SpotifyPlaybackState state);
		static Track trackFromAndroidTrack(JNIEnv* env, jni::android::spotify::SpotifyTrack track, jni::android::spotify::SpotifyMetadata metadata);
		static Metadata metadataFromAndroidMetadata(JNIEnv* env, jni::android::spotify::SpotifyMetadata metadata);
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
		void onStreamingLoginError(SpotifyError);
		void onStreamingError(SpotifyError);
		
		Options options;
		
		SpotifyAuth* auth;
		#ifdef TARGETPLATFORM_IOS
		OBJCPP_PTR(SPTAudioStreamingController) player;
		OBJCPP_PTR(SpotifyPlayerEventHandler) playerEventHandler;
		#endif
		#ifdef __ANDROID__
		JNI_PTR(jni::android::spotify::SpotifyUtils) spotifyUtils;
		JNI_PTR(jni::android::spotify::SpotifyPlayer) player;
		JNI_PTR(jni::android::spotify::SpotifyPlayerEventHandler) playerEventHandler;
		#endif
		
		bool starting;
		LinkedList<WaitCallback> startCallbacks;
		std::recursive_mutex startMutex;
		
		bool loggingIn;
		bool loggedIn;
		bool loggingOut;
		bool renewingSession;
		LinkedList<WaitCallback> loginCallbacks;
		LinkedList<WaitCallback> logoutCallbacks;
		std::mutex loginMutex;
		
		LinkedList<SpotifyPlayerEventListener*> listeners;
		std::mutex listenersMutex;
		
		#ifdef TARGETPLATFORM_IOS
		// When performing session renewal, the iOS SpotifyPlayer loses stops playing and loses its state
		//  This metadata is used to forcibly restore the state
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
		// iOS requires activating / deactivating audio sessions when audio starts and stops
		void activateAudioSession();
		void deactivateAudioSession();
		#endif
	};
}

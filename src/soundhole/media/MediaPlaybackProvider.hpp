//
//  MediaPlaybackProvider.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 12/1/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include "Track.hpp"

namespace sh {
	class MediaPlaybackProvider {
	public:
		struct State {
			bool playing;
			double position;
			bool shuffling;
			bool repeating;
		};
		
		struct Metadata {
			$<Track> previousTrack;
			$<Track> currentTrack;
			$<Track> nextTrack;
		};
		
		class EventListener {
		public:
			virtual ~EventListener() {}
			
			virtual void onMediaPlaybackProviderPlay(MediaPlaybackProvider* provider) = 0;
			virtual void onMediaPlaybackProviderPause(MediaPlaybackProvider* provider) = 0;
			virtual void onMediaPlaybackProviderTrackFinish(MediaPlaybackProvider* provider) = 0;
			virtual void onMediaPlaybackProviderMetadataChange(MediaPlaybackProvider* provider) = 0;
		};
		
		virtual ~MediaPlaybackProvider() {}
		
		virtual bool usesPublicAudioStreams() const = 0;
		
		void addEventListener(EventListener* listener);
		void removeEventListener(EventListener* listener);
		
		virtual Promise<void> prepare($<Track> track) = 0;
		virtual Promise<void> play($<Track> track, double position) = 0;
		virtual Promise<void> setPlaying(bool playing) = 0;
		virtual void stop() = 0;
		virtual Promise<void> seek(double position) = 0;
		
		virtual State state() const = 0;
		virtual Metadata metadata() const = 0;
		
	protected:
		template<typename MemberFunc, typename ...Args>
		inline void callListenerEvent(MemberFunc func, Args... args);
		
	private:
		std::mutex listenersMutex;
		LinkedList<MediaPlaybackProvider::EventListener*> listeners;
	};



#pragma mark MediaPlaybackProvider implementation

	template<typename MemberFunc, typename ...Args>
	void MediaPlaybackProvider::callListenerEvent(MemberFunc func, Args... args) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		auto listeners = this->listeners;
		lock.unlock();
		for(auto listener : listeners) {
			(listener->*func)(args...);
		}
	}
}

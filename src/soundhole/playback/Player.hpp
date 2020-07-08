//
//  Player.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 2/9/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/MediaProvider.hpp>
#include "PlaybackOrganizer.hpp"

#ifdef __OBJC__
#import <Foundation/Foundation.h>
@protocol SHPlayerEventListener;
#endif

namespace sh {
	class Player: public std::enable_shared_from_this<Player>, protected PlaybackOrganizer::Delegate, protected PlaybackOrganizer::EventListener, protected MediaPlaybackProvider::EventListener {
	public:
		using NoItem = PlaybackOrganizer::NoItem;
		using ItemVariant = PlaybackOrganizer::ItemVariant;
		
		struct State {
			bool playing;
			double position;
			bool shuffling;
		};
		
		struct Metadata {
			$<Track> previousTrack;
			$<Track> currentTrack;
			$<Track> nextTrack;
		};
		
		struct Event {
			Metadata metadata;
			State state;
			$<TrackCollection> context;
			LinkedList<$<QueueItem>> queue;
		};
		
		class EventListener {
		public:
			virtual ~EventListener() {}
			
			virtual void onPlayerStateChange($<Player> player, const Event& event) {}
			virtual void onPlayerMetadataChange($<Player> player, const Event& event) {}
			virtual void onPlayerQueueChange($<Player> player, const Event& event) {}
			virtual void onPlayerTrackFinish($<Player> player, const Event& event) {}
			
			virtual void onPlayerPlay($<Player> player, const Event& event) {}
			virtual void onPlayerPause($<Player> player, const Event& event) {}
		};
		
		struct Options {
			String savePrefix;
			double nextTrackPreloadTime = 10.0;
			double progressSaveInterval = 1.0;
		};
		
		static $<Player> new$(Options options);
		
		Player(Options);
		virtual ~Player();
		
		void addEventListener(EventListener* listener);
		void removeEventListener(EventListener* listener);
		bool hasEventListener(EventListener* listener);
		#if defined(__OBJC__) && defined(TARGETPLATFORM_IOS)
		void addEventListener(id<SHPlayerEventListener> listener);
		void removeEventListener(id<SHPlayerEventListener> listener);
		bool hasEventListener(id<SHPlayerEventListener> listener);
		#endif
		
		Promise<void> load(MediaProviderStash* stash);
		struct SaveOptions {
			bool includeMetadata = false;
		};
		Promise<void> save(SaveOptions options);
		
		Promise<void> play($<Track> track);
		Promise<void> play($<TrackCollectionItem> item);
		Promise<void> play($<QueueItem> queueItem);
		Promise<void> play(ItemVariant item);
		Promise<void> playAtQueueIndex(size_t index);
		Promise<void> seek(double position);
		Promise<bool> skipToPrevious();
		Promise<bool> skipToNext();
		
		$<QueueItem> addToQueue($<Track> track);
		void removeFromQueue($<QueueItem> queueItem);
		
		Promise<void> setPlaying(bool playing);
		void setShuffling(bool shuffling);
		
		Metadata metadata();
		State state() const;
		
		ItemVariant currentItem() const;
		
		$<TrackCollection> context() const;
		Optional<size_t> contextIndex() const;
		Optional<size_t> previousContextIndex() const;
		Optional<size_t> nextContextIndex() const;
		
		LinkedList<$<QueueItem>> queueItems() const;
		
	protected:
		virtual Promise<void> onPlaybackOrganizerPrepareTrack($<PlaybackOrganizer> organizer, $<Track> track) override;
		virtual Promise<void> onPlaybackOrganizerPlayTrack($<PlaybackOrganizer> organizer, $<Track> track) override;
		virtual void onPlaybackOrganizerTrackChange($<PlaybackOrganizer> organizer) override;
		virtual void onPlaybackOrganizerQueueChange($<PlaybackOrganizer> organizer) override;
		
		virtual void onMediaPlaybackProviderPlay(MediaPlaybackProvider* provider) override;
		virtual void onMediaPlaybackProviderPause(MediaPlaybackProvider* provider) override;
		virtual void onMediaPlaybackProviderTrackFinish(MediaPlaybackProvider* provider) override;
		virtual void onMediaPlaybackProviderMetadataChange(MediaPlaybackProvider* provider) override;
		
	private:
		String getProgressFilePath() const;
		String getMetadataFilePath() const;
		
		struct ProgressData {
			String uri;
			String providerName;
			double position;
			
			Json toJson() const;
			static ProgressData fromJson(Json json);
		};
		
		template<typename MemberFunc, typename ...Args>
		inline void callPlayerListenerEvent(MemberFunc func, Args... args);
		
		void onMediaPlaybackProviderEvent();
		
		Promise<void> performSave(SaveOptions options);
		void saveInBackground(SaveOptions options);
		
		Promise<void> prepareTrack($<Track> track);
		Promise<void> playTrack($<Track> track);
		void setMediaProvider(MediaProvider* provider);
		
		void startPlayerStateInterval();
		void stopPlayerStateInterval();
		void onPlayerStateInterval();
		
		Event createEvent();
		
		#if defined(TARGETPLATFORM_IOS)
		void deleteObjcListenerWrappers();
		#endif
		
		#if defined(TARGETPLATFORM_IOS)
		void activateAudioSession();
		void deactivateAudioSession();
		#endif
		
		Options options;
		
		$<PlaybackOrganizer> organizer;
		
		MediaProvider* mediaProvider;
		MediaProvider* preparedMediaProvider;
		
		Optional<MediaPlaybackProvider::State> providerPlaybackState;
		Optional<MediaPlaybackProvider::Metadata> providerPlaybackMetadata;
		$<Track> playingTrack;
		
		$<Timer> providerPlayerStateTimer;
		
		Optional<ProgressData> resumableProgress;
		Optional<std::chrono::steady_clock::time_point> lastSaveTime;
		
		AsyncQueue playQueue;
		AsyncQueue saveQueue;
		
		std::mutex listenersMutex;
		LinkedList<EventListener*> listeners;
	};



	#pragma mark Player implementation

	template<typename MemberFunc, typename ...Args>
	void Player::callPlayerListenerEvent(MemberFunc func, Args... args) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		auto listeners = this->listeners;
		lock.unlock();
		for(auto listener : listeners) {
			(listener->*func)(args...);
		}
	}
}




#ifdef __OBJC__

#pragma mark Player ObjC

@protocol SHPlayerEventListener <NSObject>

@optional
-(void)player:(fgl::$<sh::Player>)player didChangeState:(const sh::Player::Event&)event;
-(void)player:(fgl::$<sh::Player>)player didChangeMetadata:(const sh::Player::Event&)event;
-(void)player:(fgl::$<sh::Player>)player didChangeQueue:(const sh::Player::Event&)event;
-(void)player:(fgl::$<sh::Player>)player didFinishTrack:(const sh::Player::Event&)event;

-(void)player:(fgl::$<sh::Player>)player didPlay:(const sh::Player::Event&)event;
-(void)player:(fgl::$<sh::Player>)player didPause:(const sh::Player::Event&)event;

@end

#endif

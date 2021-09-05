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
#include <soundhole/media/PlaybackHistoryItem.hpp>
#include <soundhole/database/MediaDatabase.hpp>
#include "PlaybackOrganizer.hpp"
#include "MediaControls.hpp"

#ifdef __OBJC__
#import <Foundation/Foundation.h>
@protocol SHPlayerEventListener;
#endif

namespace sh {
	class PlayerHistoryManager;

	class Player: public std::enable_shared_from_this<Player>, protected PlaybackOrganizer::Delegate, protected PlaybackOrganizer::EventListener, protected MediaPlaybackProvider::EventListener, protected MediaControls::Listener {
		friend class PlayerHistoryManager;
	public:
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
		};
		
		class EventListener {
		public:
			virtual ~EventListener() {}
			
			virtual void onPlayerStateChange($<Player> player, const Event& event) {}
			virtual void onPlayerMetadataChange($<Player> player, const Event& event) {}
			virtual void onPlayerOrganizerItemChange($<Player> player, PlayerItem item) {}
			virtual void onPlayerQueueChange($<Player> player, const Event& event) {}
			virtual void onPlayerTrackFinish($<Player> player, const Event& event) {}
			
			virtual void onPlayerPlay($<Player> player, const Event& event) {}
			virtual void onPlayerPause($<Player> player, const Event& event) {}
			
			virtual void onPlayerCreateHistoryItem($<Player> player, $<PlaybackHistoryItem> historyItem) {}
			virtual void onPlayerUpdateHistoryItem($<Player> player, $<PlaybackHistoryItem> historyItem) {}
		};
		
		struct Options {
			String savePath;
			MediaControls* mediaControls = nullptr;
		};
		
		struct Preferences {
			double nextTrackPreloadTime = 10.0;
			double progressSaveInterval = 1.0;
			Optional<double> minDurationForHistory = 0.5;
			Optional<double> minDurationRatioForRepeatSongToBeNewHistoryItem = 0.75;
		};
		
		static $<Player> new$(MediaDatabase* database, Options options);
		
		Player(MediaDatabase* database, Options);
		virtual ~Player();
		
		Player(const Player&) = delete;
		Player& operator=(const Player&) = delete;
		
		void setPreferences(Preferences);
		const Preferences& getPreferences() const;
		
		void setOrganizerPreferences(PlaybackOrganizer::Preferences);
		const PlaybackOrganizer::Preferences& getOrganizerPreferences() const;
		
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
		Promise<void> play(PlayerItem item);
		Promise<void> playAtQueueIndex(size_t index);
		Promise<void> seek(double position);
		Promise<bool> skipToPrevious();
		Promise<bool> skipToNext();
		
		$<QueueItem> addToQueue($<Track> track);
		$<QueueItem> addToQueueFront($<Track> track);
		$<QueueItem> addToQueueRandomly($<Track> track);
		bool removeFromQueue($<QueueItem> queueItem);
		bool clearQueue();
		
		Promise<void> setPlaying(bool playing);
		void setShuffling(bool shuffling);
		
		Metadata metadata();
		State state() const;
		
		Optional<PlayerItem> currentItem() const;
		$<PlaybackHistoryItem> currentHistoryItem() const;
		
		$<TrackCollection> context() const;
		Optional<size_t> contextIndex() const;
		Optional<size_t> previousContextIndex() const;
		Optional<size_t> nextContextIndex() const;
		
		ArrayList<$<QueueItem>> queuePastItems() const;
		ArrayList<$<QueueItem>> queueItems() const;
		
		void setMediaControls(MediaControls*);
		MediaControls* getMediaControls();
		const MediaControls* getMediaControls() const;
		
	protected:
		virtual Promise<void> onPlaybackOrganizerPrepareItem($<PlaybackOrganizer> organizer, PlayerItem item) override;
		virtual Promise<void> onPlaybackOrganizerPlayItem($<PlaybackOrganizer> organizer, PlayerItem item) override;
		virtual void onPlaybackOrganizerItemChange($<PlaybackOrganizer> organizer) override;
		virtual void onPlaybackOrganizerQueueChange($<PlaybackOrganizer> organizer) override;
		
		virtual void onMediaPlaybackProviderPlay(MediaPlaybackProvider* provider) override;
		virtual void onMediaPlaybackProviderPause(MediaPlaybackProvider* provider) override;
		virtual void onMediaPlaybackProviderTrackFinish(MediaPlaybackProvider* provider) override;
		virtual void onMediaPlaybackProviderMetadataChange(MediaPlaybackProvider* provider) override;
		
		virtual MediaControls::HandlerStatus onMediaControlsPause() override;
		virtual MediaControls::HandlerStatus onMediaControlsPlay() override;
		virtual MediaControls::HandlerStatus onMediaControlsStop() override;
		virtual MediaControls::HandlerStatus onMediaControlsPrevious() override;
		virtual MediaControls::HandlerStatus onMediaControlsNext() override;
		virtual MediaControls::HandlerStatus onMediaControlsChangeRepeatMode(MediaControls::RepeatMode) override;
		virtual MediaControls::HandlerStatus onMediaControlsChangeShuffleMode(bool) override;
		
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
		
		Promise<void> prepareItem(PlayerItem item);
		Promise<void> playItem(PlayerItem item);
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
		
		void updateMediaControls();
		
		MediaDatabase* database;
		
		Options options;
		Preferences prefs;
		
		$<PlaybackOrganizer> organizer;
		
		MediaProvider* mediaProvider;
		MediaProvider* preparedMediaProvider;
		
		Optional<MediaPlaybackProvider::State> providerPlaybackState;
		Optional<MediaPlaybackProvider::Metadata> providerPlaybackMetadata;
		Optional<PlayerItem> playingItem;
		
		$<Timer> providerPlayerStateTimer;
		
		Optional<ProgressData> resumableProgress;
		Optional<std::chrono::steady_clock::time_point> lastSaveTime;
		
		AsyncQueue playQueue;
		AsyncQueue saveQueue;
		
		std::mutex listenersMutex;
		LinkedList<EventListener*> listeners;
		
		PlayerHistoryManager* historyManager;
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

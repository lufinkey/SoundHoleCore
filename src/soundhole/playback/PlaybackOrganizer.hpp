//
//  PlaybackOrganizer.hpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/26/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#pragma once

#include <soundhole/common.hpp>
#include <soundhole/media/Track.hpp>
#include <soundhole/media/TrackCollection.hpp>
#include <soundhole/media/QueueItem.hpp>
#include <soundhole/media/PlayerItem.hpp>
#include "PlaybackQueue.hpp"

namespace sh {
	class PlaybackOrganizer: public std::enable_shared_from_this<PlaybackOrganizer> {
	public:
		class Delegate {
		public:
			virtual ~Delegate() {}
			
			virtual Promise<void> onPlaybackOrganizerPrepareItem($<PlaybackOrganizer> organizer, PlayerItem item) = 0;
			virtual Promise<void> onPlaybackOrganizerPlayItem($<PlaybackOrganizer> organizer, PlayerItem item) = 0;
		};
		
		class EventListener {
		public:
			virtual ~EventListener() {}
			
			virtual void onPlaybackOrganizerItemChange($<PlaybackOrganizer> organizer) {}
			virtual void onPlaybackOrganizerQueueChange($<PlaybackOrganizer> organizer) {}
		};
		
		struct Preferences {
			size_t contextLoadBuffer = 5;
			
			bool pastQueueEnabled = true;
			bool savePastQueueToDisk = false;
			
			bool clearPastQueueOnExitQueue = false;
			bool clearPastQueueOnNextAfterExitQueue = true;
			bool clearPastQueueOnContextChange = false;
		};
		
		static $<PlaybackOrganizer> new$(Delegate* delegate);
		
		PlaybackOrganizer(Delegate* delegate);
		
		void setPreferences(Preferences);
		const Preferences& getPreferences() const;
		
		void addEventListener(EventListener* listener);
		void removeEventListener(EventListener* listener);
		
		Promise<void> save(String path);
		Promise<bool> load(String path, MediaProviderStash* stash);
		
		Promise<bool> previous();
		Promise<bool> next();
		Promise<void> prepareNextIfNeeded();
		Promise<void> play($<QueueItem> item);
		Promise<void> play($<TrackCollectionItem> item);
		Promise<void> play($<Track> track);
		Promise<void> play(PlayerItem item);
		Promise<void> setCurrentItem(PlayerItem item);
		Promise<void> stop();
		
		$<QueueItem> addToQueue($<Track> track);
		$<QueueItem> addToQueueFront($<Track> track);
		$<QueueItem> addToQueueRandomly($<Track> track);
		bool removeFromQueue($<QueueItem> item);
		bool clearQueue();
		
		Optional<PlayerItem> getCurrentItem() const;
		Promise<Optional<PlayerItem>> getPreviousItem();
		Promise<Optional<PlayerItem>> getNextItem();
		
		$<Track> getCurrentTrack() const;
		Promise<$<Track>> getPreviousTrack();
		Promise<$<Track>> getNextTrack();
		
		$<QueueItem> getPreviousInQueue() const;
		$<QueueItem> getNextInQueue() const;
		Promise<$<TrackCollectionItem>> getPreviousInContext();
		Promise<$<TrackCollectionItem>> getNextInContext();
		
		$<TrackCollection> getContext() const;
		Optional<size_t> getContextIndex() const;
		Optional<size_t> getPreviousContextIndex() const;
		Optional<size_t> getNextContextIndex() const;
		ArrayList<$<QueueItem>> getQueuePastItems() const;
		ArrayList<$<QueueItem>> getQueueItems() const;
		$<QueueItem> getQueuePastItem(size_t index) const;
		$<QueueItem> getQueueItem(size_t index) const;
		size_t getQueuePastItemCount() const;
		size_t getQueueItemCount() const;
		
		void setShuffling(bool shuffling);
		bool isShuffling() const;
		
		bool isPreparing() const;
		bool hasPreparedNext() const;
		
	private:
		void updateMainContext($<TrackCollection> context, $<TrackCollectionItem> contextItem, bool shuffling);
		Promise<void> prepareCollectionTracks($<TrackCollection> collection, size_t index);
		
		struct SetPlayingOptions {
			enum class Trigger {
				PLAY,
				NEXT,
				PREVIOUS
			};
			
			Trigger trigger = Trigger::PLAY;
		};
		Promise<void> setPlayingItem(Function<Promise<Optional<PlayerItem>>()> itemGetter, SetPlayingOptions options);
		Promise<void> applyPlayingItem(PlayerItem item);
		
		Promise<Optional<PlayerItem>> getValidPreviousItem();
		Promise<Optional<PlayerItem>> getValidNextItem();
		
		Delegate* delegate;
		Preferences prefs;
		
		$<TrackCollection> context;
		$<TrackCollectionItem> contextItem;
		TrackCollection::ItemIndexMarker sourceContextIndex;
		TrackCollection::ItemIndexMarker shuffledContextIndex;
		bool shuffling;
		
		bool preparedNext;
		Optional<PlayerItem> applyingItem;
		Optional<PlayerItem> playingItem;
		
		PlaybackQueue queue;
		
		AsyncQueue playQueue;
		AsyncQueue continuousPlayQueue;
		
		std::mutex listenersMutex;
		LinkedList<EventListener*> listeners;
	};
}

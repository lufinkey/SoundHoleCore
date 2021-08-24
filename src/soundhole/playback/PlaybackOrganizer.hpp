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
#include "PlaybackQueue.hpp"

namespace sh {
	class PlaybackOrganizer: public std::enable_shared_from_this<PlaybackOrganizer> {
	public:
		using NoItem = std::nullptr_t;
		using ItemVariant = std::variant<NoItem,$<Track>,$<TrackCollectionItem>,$<QueueItem>>;
		
		class Delegate {
		public:
			virtual ~Delegate() {}
			
			virtual Promise<void> onPlaybackOrganizerPrepareTrack($<PlaybackOrganizer> organizer, $<Track> track) = 0;
			virtual Promise<void> onPlaybackOrganizerPlayTrack($<PlaybackOrganizer> organizer, $<Track> track) = 0;
		};
		
		class EventListener {
		public:
			virtual ~EventListener() {}
			
			virtual void onPlaybackOrganizerTrackChange($<PlaybackOrganizer> organizer) {}
			virtual void onPlaybackOrganizerQueueChange($<PlaybackOrganizer> organizer) {}
		};
		
		struct Options {
			Delegate* delegate = nullptr;
			size_t contextLoadBuffer = 5;
		};
		
		struct Preferences {
			bool pastQueueEnabled = true;
			bool savePastQueueToDisk = false;
			
			bool clearPastQueueOnExitQueue = false;
			bool clearPastQueueOnNextAfterExitQueue = true;
			bool clearPastQueueOnContextChange = false;
		};
		
		static $<PlaybackOrganizer> new$(Options options);
		
		PlaybackOrganizer(Options options);
		
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
		Promise<void> play(ItemVariant item);
		Promise<void> setCurrentItem(ItemVariant item);
		Promise<void> stop();
		
		$<QueueItem> addToQueue($<Track> track);
		$<QueueItem> addToQueueFront($<Track> track);
		$<QueueItem> addToQueueRandomly($<Track> track);
		bool removeFromQueue($<QueueItem> item);
		
		ItemVariant getCurrentItem() const;
		Promise<ItemVariant> getPreviousItem();
		Promise<ItemVariant> getNextItem();
		
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
		
		static $<Track> trackFromItem(ItemVariant item);
		
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
			bool apply = true;
		};
		Promise<void> setPlayingItem(Function<Promise<ItemVariant>()> itemGetter, SetPlayingOptions options);
		Promise<void> applyPlayingItem(ItemVariant item);
		
		Promise<ItemVariant> getValidPreviousItem();
		Promise<ItemVariant> getValidNextItem();
			
		static Json itemToJson(ItemVariant itemVariant);
		
		Options options;
		Preferences prefs;
		
		$<TrackCollection> context;
		$<TrackCollectionItem> contextItem;
		TrackCollection::ItemIndexMarker sourceContextIndex;
		TrackCollection::ItemIndexMarker shuffledContextIndex;
		bool shuffling;
		
		bool preparedNext;
		std::variant<NoItem,$<Track>,$<TrackCollectionItem>,$<QueueItem>> applyingItem;
		std::variant<NoItem,$<Track>,$<TrackCollectionItem>,$<QueueItem>> playingItem;
		
		PlaybackQueue queue;
		
		AsyncQueue playQueue;
		AsyncQueue continuousPlayQueue;
		
		std::mutex listenersMutex;
		LinkedList<EventListener*> listeners;
	};
}

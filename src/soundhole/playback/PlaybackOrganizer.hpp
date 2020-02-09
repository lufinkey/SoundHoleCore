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

namespace sh {
	class PlaybackOrganizer: public std::enable_shared_from_this<PlaybackOrganizer> {
	public:
		struct NoItem {
			inline bool operator==(const NoItem&) const {
				return true;
			}
		};
		using ItemVariant = std::variant<NoItem,$<Track>,$<TrackCollectionItem>,$<QueueItem>>;
		
		class Delegate {
		public:
			virtual ~Delegate() {}
			
			virtual Promise<void> onPlaybackOrganizerPlayTrack(PlaybackOrganizer* organizer, $<Track> track) = 0;
			virtual Promise<void> onPlaybackOrganizerPrepareTrack(PlaybackOrganizer* organizer, $<Track> track) = 0;
		};
		
		struct Options {
			Delegate* delegate = nullptr;
			size_t contextLoadBuffer = 5;
		};
		
		PlaybackOrganizer(Options options);
		
		Promise<void> save(String path);
		Promise<bool> load(String path);
		
		Promise<bool> previous();
		Promise<bool> next();
		Promise<void> prepareNext();
		Promise<void> play($<QueueItem> item);
		Promise<void> play($<TrackCollectionItem> item);
		Promise<void> play($<Track> track);
		Promise<void> stop();
		
		$<QueueItem> addToQueue($<Track> track);
		void removeFromQueue($<QueueItem> item);
		
		ItemVariant getCurrentItem();
		Promise<ItemVariant> getPreviousItem();
		Promise<ItemVariant> getNextItem();
		
		$<Track> getCurrentTrack();
		Promise<$<Track>> getPreviousTrack();
		Promise<$<Track>> getNextTrack();
		
		$<QueueItem> getNextInQueue();
		Promise<$<TrackCollectionItem>> getPreviousInContext();
		Promise<$<TrackCollectionItem>> getNextInContext();
		
		$<TrackCollection> getContext() const;
		Optional<size_t> getContextIndex() const;
		LinkedList<$<QueueItem>> getQueue() const;
		$<QueueItem> getQueueItem(size_t index) const;
		size_t getQueueLength() const;
		
		void setShuffling(bool shuffling);
		bool isShuffling() const;
		
		static $<Track> trackFromItem(ItemVariant item);
		
	private:
		void updateMainContext($<TrackCollection> context, $<TrackCollectionItem> contextItem, bool shuffling);
		Promise<void> prepareCollectionTracks($<TrackCollection> collection, size_t index);
		Promise<void> setPlayingItem(Function<Promise<ItemVariant>()> itemGetter);
		Promise<void> applyPlayingItem(ItemVariant item);
		
		Promise<ItemVariant> getValidPreviousItem();
		Promise<ItemVariant> getValidNextItem();
		
		Options options;
		
		$<TrackCollection> context;
		$<TrackCollectionItem> contextItem;
		Optional<size_t> sourceContextIndex;
		Optional<size_t> shuffledContextIndex;
		bool shuffling;
		
		bool preparedNext;
		std::variant<NoItem,$<Track>,$<TrackCollectionItem>,$<QueueItem>> applyingItem;
		std::variant<NoItem,$<Track>,$<TrackCollectionItem>,$<QueueItem>> playingItem;
		
		LinkedList<$<QueueItem>> queue;
		
		AsyncQueue playQueue;
		AsyncQueue continuousPlayQueue;
	};
}

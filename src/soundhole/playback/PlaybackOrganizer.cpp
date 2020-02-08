//
//  PlaybackOrganizer.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/26/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "PlaybackOrganizer.hpp"
#include <soundhole/media/ShuffledTrackCollection.hpp>
#include <soundhole/utils/Utils.hpp>

namespace sh {
	PlaybackOrganizer::PlaybackOrganizer(Options options)
	: options(options),
	shuffling(false), preparedNext(false),
	applyingItem(NoItem()), playingItem(NoItem()) {
		//
	}



	$<Track> PlaybackOrganizer::trackFromItem(ItemVariant item) {
		switch(item.index()) {
			case 0:
				std::get<NoItem>(item);
				return nullptr;
				
			case 1:
				return std::get<$<Track>>(item);
				
			case 2:
				return std::get<$<TrackCollectionItem>>(item)->track();
				
			case 3:
				return std::get<$<QueueItem>>(item)->track();
		}
		throw std::logic_error("invalid state for current item");
	}



	Promise<bool> PlaybackOrganizer::previous() {
		auto self = shared_from_this();
		return setPlayingItem([=]() {
			return self->getValidPreviousItem();
		});
	}

	Promise<bool> PlaybackOrganizer::next() {
		auto self = shared_from_this();
		return setPlayingItem([=]() {
			return self->getValidNextItem();
		});
	}

	Promise<void> PlaybackOrganizer::prepareNext() {
		if(continuousPlayQueue.getTaskWithTag("prepare")) {
			return continuousPlayQueue.waitForTasksWithTag("prepare");
		}
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "prepare",
			.cancelMatchingTags = true
		};
		auto self = shared_from_this();
		return continuousPlayQueue.run(runOptions, [=](auto task) {
			if(self->preparedNext) {
				return generate_items<void>({});
			}
			struct SharedData {
				ItemVariant nextItem = NoItem();
			};
			auto sharedData = new$<SharedData>();
			return generate_items<void>({
				[=]() {
					return self->getNextItem().then([=](ItemVariant nextItem) {
						sharedData->nextItem = nextItem;
					});
				},
				[=]() {
					auto nextItem = sharedData->nextItem;
					if(nextItem.index() == 0) {
						return Promise<void>::resolve();
					}
					auto nextTrack = trackFromItem(nextItem);
					if(self->options.delegate == nullptr) {
						self->preparedNext = true;
						return Promise<void>::resolve();
					} else {
						return self->options.delegate->onPlaybackOrganizerPrepareTrack(this, nextTrack).then([=]() {
							self->preparedNext = true;
						});
					}
				}
			});
		}).promise;
	}
	
	Promise<void> PlaybackOrganizer::play($<QueueItem> item) {
		FGL_ASSERT(item != nullptr, "item cannot be null");
		return setPlayingItem([=]() {
			return Promise<ItemVariant>::resolve(item);
		}).toVoid();
	}

	Promise<void> PlaybackOrganizer::play($<TrackCollectionItem> item) {
		FGL_ASSERT(item != nullptr, "item cannot be null");
		return setPlayingItem([=]() {
			return Promise<ItemVariant>::resolve(item);
		}).toVoid();
	}

	Promise<void> PlaybackOrganizer::play($<Track> track) {
		FGL_ASSERT(track != nullptr, "track cannot be null");
		return setPlayingItem([=]() {
			return Promise<ItemVariant>::resolve(track);
		}).toVoid();
	}



	$<QueueItem> PlaybackOrganizer::addToQueue($<Track> track) {
		auto queueItem = QueueItem::new$(track);
		queue.pushBack(queueItem);
		// TODO emit queueChange event
		return queueItem;
	}

	void PlaybackOrganizer::removeFromQueue($<QueueItem> item) {
		auto queueIt = queue.findEqual(item);
		if(queueIt != queue.end()) {
			queue.erase(queueIt);
			// TODO emit queueChange event
		}
	}



	PlaybackOrganizer::ItemVariant PlaybackOrganizer::getCurrentItem() {
		if(applyingItem.index() != 0) {
			return applyingItem;
		} else if(playingItem.index() != 0) {
			return playingItem;
		}
		return NoItem();
	}

	Promise<PlaybackOrganizer::ItemVariant> PlaybackOrganizer::getPreviousItem() {
		return getPreviousInContext().map<ItemVariant>(nullptr, []($<TrackCollectionItem> item) -> ItemVariant {
			if(!item) {
				return NoItem();
			}
			return item;
		});
	}

	Promise<PlaybackOrganizer::ItemVariant> PlaybackOrganizer::getNextItem() {
		auto queueItem = getNextInQueue();
		if(queueItem) {
			return Promise<ItemVariant>::resolve(queueItem);
		}
		auto self = shared_from_this();
		return getNextInContext().map<ItemVariant>([=]($<TrackCollectionItem> item) -> ItemVariant {
			if(item) {
				return item;
			}
			auto queueItem = self->getNextInQueue();
			if(queueItem) {
				return queueItem;
			}
			return NoItem();
		});
	}

	Promise<PlaybackOrganizer::ItemVariant> PlaybackOrganizer::getValidPreviousItem() {
		auto self = shared_from_this();
		auto currentContext = this->context;
		return getPreviousItem().then([=](ItemVariant item) -> Promise<ItemVariant> {
			if(self->context != currentContext) {
				return getValidPreviousItem();
			}
			return Promise<ItemVariant>::resolve(item);
		});
	}

	Promise<PlaybackOrganizer::ItemVariant> PlaybackOrganizer::getValidNextItem() {
		auto self = shared_from_this();
		auto currentContext = this->context;
		return getNextItem().then([=](ItemVariant item) -> Promise<ItemVariant> {
			if(self->context != currentContext) {
				return getValidNextItem();
			}
			return Promise<ItemVariant>::resolve(item);
		});
	}



	$<Track> PlaybackOrganizer::getCurrentTrack() {
		return trackFromItem(getCurrentItem());
	}

	Promise<$<Track>> PlaybackOrganizer::getPreviousTrack() {
		return getPreviousItem().map<$<Track>>(nullptr, [](ItemVariant item) {
			return trackFromItem(item);
		});
	}

	Promise<$<Track>> PlaybackOrganizer::getNextTrack() {
		return getNextItem().map<$<Track>>(nullptr, [](ItemVariant item) {
			return trackFromItem(item);
		});
	}



	$<QueueItem> PlaybackOrganizer::getNextInQueue() {
		if(queue.size() == 0) {
			return nullptr;
		}
		return queue.front();
	}

	Promise<$<TrackCollectionItem>> PlaybackOrganizer::getPreviousInContext() {
		if(!context || !contextItem) {
			return Promise<$<TrackCollectionItem>>::resolve(nullptr);
		}
		auto optContextIndex = contextItem->indexInContext();
		if(!optContextIndex) {
			optContextIndex = getContextIndex();
			if(!optContextIndex) {
				return Promise<$<TrackCollectionItem>>::resolve(nullptr);
			}
		}
		size_t contextIndex = optContextIndex.value();
		if(contextIndex == 0) {
			return Promise<$<TrackCollectionItem>>::resolve(nullptr);
		}
		size_t prevIndex = contextIndex - 1;
		auto self = shared_from_this();
		return context->getItem(prevIndex);
	}

	Promise<$<TrackCollectionItem>> PlaybackOrganizer::getNextInContext() {
		if(!context || !contextItem) {
			return Promise<$<TrackCollectionItem>>::resolve(nullptr);
		}
		size_t nextIndex = -1;
		auto optContextIndex = contextItem->indexInContext();
		if(!optContextIndex) {
			optContextIndex = getContextIndex();
			if(!optContextIndex) {
				return Promise<$<TrackCollectionItem>>::resolve(nullptr);
			} else {
				nextIndex = optContextIndex.value();
			}
		} else {
			nextIndex = optContextIndex.value() + 1;
		}
		return context->getItem(nextIndex);
	}



	$<TrackCollection> PlaybackOrganizer::getContext() const {
		return context;
	}

	Optional<size_t> PlaybackOrganizer::getContextIndex() const {
		if(std::dynamic_pointer_cast<ShuffledTrackCollectionItem>(contextItem) != nullptr) {
			return shuffledContextIndex;
		} else {
			return sourceContextIndex;
		}
	}

	LinkedList<$<QueueItem>> PlaybackOrganizer::getQueue() const {
		return queue;
	}

	$<QueueItem> PlaybackOrganizer::getQueueItem(size_t index) const {
		if(index >= queue.size()) {
			throw std::invalid_argument("invalid queue index "+std::to_string(index));
		}
		return *std::next(queue.begin(), index);
	}

	size_t PlaybackOrganizer::getQueueLength() const {
		return queue.size();
	}



	void PlaybackOrganizer::setShuffling(bool shuffling) {
		updateMainContext(context, contextItem, shuffling);
	}

	bool PlaybackOrganizer::isShuffling() const {
		return shuffling;
	}



	Promise<void> PlaybackOrganizer::prepareCollectionTracks($<TrackCollection> collection, size_t index) {
		size_t startIndex = index;
		if(startIndex <= options.contextLoadBuffer) {
			startIndex = 0;
		}
		size_t endIndex = index + options.contextLoadBuffer;
		return collection->loadItems(startIndex, (endIndex - startIndex));
	}

	Promise<bool> PlaybackOrganizer::setPlayingItem(Function<Promise<ItemVariant>()> itemGetter) {
		return Promise<bool>([=](auto resolve, auto reject) {
			playQueue.run({.cancelAll=true}, [=](auto task) {
				return itemGetter().then([=](ItemVariant item) -> Promise<bool> {
					bool queueChanged = false;
					if(std::get_if<NoItem>(&item)) {
						return Promise<bool>::resolve(false);
					}
					else if(auto queueItemPtr = std::get_if<$<QueueItem>>(&item)) {
						auto queueItem = *queueItemPtr;
						auto queueIt = this->queue.findEqual(queueItem);
						if(queueIt != this->queue.end()) {
							this->queue.erase(queue.begin(), std::next(queueIt, 1));
							queueChanged = true;
						}
					}
					else if(auto collectionItemPtr = std::get_if<$<TrackCollectionItem>>(&item)) {
						auto collectionItem = *collectionItemPtr;
						auto collection = collectionItem->context().lock();
						if(!collection) {
							throw std::logic_error("TrackCollectionItem has null context");
						}
						updateMainContext(collection, collectionItem, this->shuffling);
						// TODO figure out if this code below is actually necessary
						auto contextItem = this->contextItem;
						FGL_ASSERT(contextItem != nullptr, "contextItem should not be null after call to updateMainContext");
						item = contextItem;
					}
					else if(auto trackPtr = std::get_if<$<Track>>(&item)) {
						updateMainContext(nullptr, nullptr, this->shuffling);
					}
					applyingItem = item;
					applyPlayingItem(item);
					// TODO emit trackChange
					if(queueChanged) {
						// TODO emit queueChange
					}
					return Promise<bool>::resolve(true);
				}).then(resolve, reject);
			});
		});
	}

	Promise<void> PlaybackOrganizer::applyPlayingItem(ItemVariant item) {
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "play",
			.cancelTags = { "prepare", "play" }
		};
		auto self = shared_from_this();
		return continuousPlayQueue.run(runOptions, [=](auto task) {
			return generate<void>([=](auto yield) {
				size_t sleepCount = 1;
				auto track = trackFromItem(item);
				while(true) {
					try {
						if(self->options.delegate != nullptr) {
							await(self->options.delegate->onPlaybackOrganizerPlayTrack(self.get(), track));
						}
						await(Promise<void>::resolve().then([&]() {
							self->playingItem = item;
							self->applyingItem = NoItem();
							self->preparedNext = false;
						}));
						yield();
						if(track->isPlayable()) {
							// load surrounding context until success
							while(true) {
								try {
									await(Promise<void>::resolve().then([&]() -> Promise<void> {
										auto context = self->context;
										auto contextIndex = self->getContextIndex();
										if(!context || !contextIndex) {
											return Promise<void>::resolve();
										}
										return self->prepareCollectionTracks(context, contextIndex.value());
									}));
									break;
								}
								catch(...) {
									printf("Failed to load next tracks in context: %s\n", utils::getExceptionDetails(std::current_exception()).c_str());
									// ignore error and try again
									yield();
									for(size_t i=0; i<10; i++) {
										std::this_thread::sleep_for(std::chrono::milliseconds(100));
										yield();
									}
								}
							}
						}
						else {
							// load next track until success
							while(true) {
								try {
									await(self->next());
									break;
								}
								catch(...) {
									printf("Failed to skip to next track after %s: %s\n",
										   track->name().c_str(), utils::getExceptionDetails(std::current_exception()).c_str());
									// ignore error and try again
									yield();
									for(size_t i=0; i<4; i++) {
										std::this_thread::sleep_for(std::chrono::milliseconds(100));
										yield();
									}
								}
							}
						}
					}
					catch(...) {
						printf("Failed to play track %s: %s\n",
							   track->name().c_str(), utils::getExceptionDetails(std::current_exception()).c_str());
						// ignore error and try again
						yield();
						for(size_t i=0; i<sleepCount; i++) {
							std::this_thread::sleep_for(std::chrono::milliseconds(100));
							yield();
						}
					}
				}
			});
		}).promise;
	}

	void PlaybackOrganizer::updateMainContext($<TrackCollection> context, $<TrackCollectionItem> contextItem, bool shuffling) {
		FGL_ASSERT((context != nullptr) == (contextItem != nullptr), "context and contextItem must both be null or non-null");
		
		auto prevContext = this->context;
		auto prevContextItem = this->contextItem;
		$<TrackCollection> prevSourceContext;
		$<TrackCollectionItem> prevSourceContextItem;
		$<ShuffledTrackCollection> prevShuffledContext;
		$<ShuffledTrackCollectionItem> prevShuffledContextItem;
		bool wasShuffling = this->shuffling;
		if(prevContext) {
			if(wasShuffling) {
				auto shuffledContext = std::static_pointer_cast<ShuffledTrackCollection>(prevContext);
				auto shuffledContextItem = std::static_pointer_cast<ShuffledTrackCollectionItem>(prevContextItem);
				prevSourceContext = shuffledContext->source();
				prevSourceContextItem = shuffledContextItem->sourceItem();
				prevShuffledContext = shuffledContext;
				prevShuffledContextItem = shuffledContextItem;
			} else {
				prevSourceContext = prevContext;
				prevSourceContextItem = prevContextItem;
				prevShuffledContext = nullptr;
				prevShuffledContextItem = nullptr;
			}
		}
		
		$<TrackCollection> sourceContext;
		$<TrackCollectionItem> sourceContextItem;
		$<ShuffledTrackCollection> shuffledContext;
		$<ShuffledTrackCollectionItem> shuffledContextItem;
		
		Optional<size_t> contextIndex;
		Optional<size_t> sourceContextIndex;
		Optional<size_t> shuffledContextIndex;
		
		if(context) {
			if(shuffling) {
				if(auto castShuffledContext = std::dynamic_pointer_cast<ShuffledTrackCollection>(context)) {
					auto castShuffledItem = std::dynamic_pointer_cast<ShuffledTrackCollectionItem>(contextItem);
					FGL_ASSERT(castShuffledItem != nullptr, "contextItem of ShuffledTrackCollection must be a ShuffledTrackCollectionItem");
					sourceContext = shuffledContext->source();
					sourceContextItem = shuffledContextItem->sourceItem();
					shuffledContext = castShuffledContext;
					shuffledContextItem = castShuffledItem;
				}
				else {
					sourceContext = context;
					sourceContextItem = contextItem;
					shuffledContext = ShuffledTrackCollection::new$(context, { contextItem });
					shuffledContextItem = std::static_pointer_cast<ShuffledTrackCollectionItem>(shuffledContext->itemAt(0));
					FGL_ASSERT(shuffledContextItem != nullptr, "First item in shuffled context cannot be null");
					context = shuffledContext;
					contextItem = shuffledContextItem;
				}
				sourceContextIndex = shuffledContextItem->sourceItem()->indexInContext();
				if(!sourceContextIndex && prevSourceContext == sourceContext) {
					sourceContextIndex = this->sourceContextIndex;
				}
				shuffledContextIndex = shuffledContextItem->indexInContext();
				if(!shuffledContextIndex && (prevContext == context)) {
					shuffledContextIndex = this->shuffledContextIndex;
				}
				contextIndex = shuffledContextIndex;
			}
			else {
				if(auto castShuffledContext = std::dynamic_pointer_cast<ShuffledTrackCollection>(context)) {
					auto castShuffledItem = std::dynamic_pointer_cast<ShuffledTrackCollectionItem>(contextItem);
					FGL_ASSERT(castShuffledItem != nullptr, "contextItem of ShuffledTrackCollection must be a ShuffledTrackCollectionItem");
					context = castShuffledContext->source();
					contextItem = castShuffledItem->sourceItem();
					sourceContext = context;
					sourceContextItem = contextItem;
					shuffledContext = nullptr;
					shuffledContextItem = nullptr;
				}
				else {
					sourceContext = context;
					sourceContextItem = contextItem;
					shuffledContext = nullptr;
					shuffledContextItem = nullptr;
				}
				FGL_ASSERT(contextItem->context().lock() == context, "contextItem must belong to context");
				sourceContextIndex = contextItem->indexInContext();
				if(!sourceContextIndex && prevSourceContext == context) {
					sourceContextIndex = this->sourceContextIndex;
				}
				shuffledContextIndex = std::nullopt;
				contextIndex = sourceContextIndex;
			}
		}
		
		if(sourceContext != prevSourceContext) {
			// TODO subscribe / unsubscribe from context changes
		}
		if(shuffledContext != prevShuffledContext) {
			// TODO subscribe / unsubscribe from context changes
		}
		
		if(context && context != this->context && contextIndex) {
			// TODO log error in logger
			prepareCollectionTracks(context, contextIndex.value()).except([=](std::exception_ptr error) {
				printf("Error preparing collection tracks: %s\n", utils::getExceptionDetails(error).c_str());
			});
		}
		
		this->context = context;
		this->contextItem = contextItem;
		this->sourceContextIndex = sourceContextIndex;
		this->shuffledContextIndex = shuffledContextIndex;
		this->shuffling = shuffling;
		if(this->playingItem.index() == 0 && this->applyingItem.index() == 0 && contextItem) {
			this->applyingItem = contextItem;
		}
		else if(sourceContextItem && prevSourceContextItem && prevSourceContextItem == sourceContextItem) {
			if(this->applyingItem.index() != 0
			   && (this->applyingItem == ItemVariant(prevSourceContextItem) || this->applyingItem == ItemVariant(prevShuffledContextItem))) {
				this->applyingItem = contextItem;
			}
			else if(this->playingItem.index() != 0
			   && (this->playingItem == ItemVariant(prevSourceContextItem) || this->playingItem == ItemVariant(prevShuffledContextItem))) {
				this->playingItem = contextItem;
			}
		}
	}
}

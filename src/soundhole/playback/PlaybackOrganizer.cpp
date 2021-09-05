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
	$<PlaybackOrganizer> PlaybackOrganizer::new$(Delegate* delegate) {
		return fgl::new$<PlaybackOrganizer>(delegate);
	}



	PlaybackOrganizer::PlaybackOrganizer(Delegate* delegate)
	: delegate(delegate),
	shuffling(false), preparedNext(false),
	applyingItem(std::nullopt), playingItem(std::nullopt) {
		//
	}


	void PlaybackOrganizer::setPreferences(Preferences prefs) {
		this->prefs = prefs;
	}

	const PlaybackOrganizer::Preferences& PlaybackOrganizer::getPreferences() const {
		return this->prefs;
	}



	Promise<void> PlaybackOrganizer::save(String path) {
		auto currentItem = getCurrentItem();
		if(currentItem.hasValue()) {
			if(auto contextItemPtr = std::get_if<$<TrackCollectionItem>>(&currentItem.value())) {
				auto contextItem = *contextItemPtr;
				if(auto shuffledItem = std::dynamic_pointer_cast<ShuffledTrackCollectionItem>(contextItem)) {
					currentItem = shuffledItem->sourceItem();
				}
			}
		}
		auto context = this->context;
		if(auto shuffledContext = std::dynamic_pointer_cast<ShuffledTrackCollection>(context)) {
			context = shuffledContext->source();
		}
		size_t tracksOffset = 0;
		size_t tracksLimit = 20;
		if(sourceContextIndex) {
			if(sourceContextIndex->index <= 10) {
				tracksOffset = 0;
			} else {
				tracksOffset = sourceContextIndex->index - 10;
			}
		}
		auto contextJson = context ? context->toJson({
			.itemsStartIndex = tracksOffset,
			.itemsLimit = tracksLimit
		}) : Json();
		// add current item to context if needed
		if(!contextJson.is_null() && sourceContextIndex && currentItem.hasValue() && std::get_if<$<TrackCollectionItem>>(&currentItem.value())) {
			auto contextItem = std::get<$<TrackCollectionItem>>(currentItem.value());
			auto itemsJson = contextJson["items"].object_items();
			itemsJson.insert_or_assign(std::to_string(sourceContextIndex->index), contextItem->toJson());
			auto contextMap = contextJson.object_items();
			contextMap.insert_or_assign("items", itemsJson);
			contextJson = Json(contextMap);
		}
		// get past queue items if needed
		Json queuePast;
		if(prefs.savePastQueueToDisk) {
			// get past queue items
			size_t queuePastItemsEndIndex = queue.pastItems.size();
			if(currentItem.hasValue() && std::get_if<$<QueueItem>>(&currentItem.value())
			   && queue.pastItems.size() > 0 && std::get<$<QueueItem>>(currentItem.value()) == queue.pastItems.back()) {
				// since current item is the last queue item, disclude that item from queuePast
				queuePastItemsEndIndex -= 1;
			}
			Json::array queuePastArr;
			queuePastArr.reserve(queuePastItemsEndIndex);
			for(auto& queueItem : queue.pastItems) {
				if(queuePastArr.size() >= queuePastItemsEndIndex) {
					break;
				}
				queuePastArr.push_back(queueItem->toJson());
			}
			queuePast = queuePastArr;
		}
		// create json and save to file
		auto json = Json(Json::object{
			{ "currentItem", currentItem ? currentItem->toJson() : Json() },
			{ "context", contextJson },
			{ "contextIndex", sourceContextIndex ? Json((double)sourceContextIndex->index) : Json() },
			{ "queuePast", queuePast },
			{ "queue", queue.items.map([&](auto& queueItem) -> Json {
				return queueItem->toJson();
			}) },
			{ "shuffling", shuffling }
		});
		return promiseThread([=]() {
			fs::writeFile(path, json.dump());
		});
	}

	Promise<bool> PlaybackOrganizer::load(String path, MediaProviderStash* stash) {
		return promiseThread([=]() {
			try {
				if(!fs::exists(path)) {
					return Json();
				}
				auto data = fs::readFile(path);
				std::string error;
				return Json::parse(data, error);
			} catch(...) {
				return Json();
			}
		}).map([=](Json json) -> bool {
			if(json.is_null()) {
				return false;
			}
			auto currentItemJson = json["currentItem"];
			auto contextJson = json["context"];
			auto contextIndexJson = json["contextIndex"];
			auto queuePastJson = json["queuePast"];
			auto queueJson = json["queue"];
			auto shuffling = json["shuffling"].bool_value();
			// get current item
			auto context = contextJson.is_null() ? $<TrackCollection>()
				: std::dynamic_pointer_cast<TrackCollection>(
					stash->parseMediaItem(contextJson));
			$<TrackCollectionItem> contextItem;
			Optional<PlayerItem> currentItem = std::nullopt;
			auto itemType = currentItemJson["type"].string_value();
			if(itemType == "collectionItem") {
				auto trackUri = currentItemJson["track"]["uri"].string_value();
				if(!contextIndexJson.is_null()) {
					auto cmpContextItem = context->itemAt((size_t)contextIndexJson.number_value());
					if(cmpContextItem && cmpContextItem->track()->uri() == trackUri) {
						contextItem = cmpContextItem;
					} else {
						contextItem = context->createCollectionItem(currentItemJson, stash);
					}
				} else {
					contextItem = context->createCollectionItem(currentItemJson, stash);
				}
				currentItem = contextItem;
			} else if(itemType == "queueItem") {
				currentItem = QueueItem::fromJson(currentItemJson, stash);
			} else if(itemType == "track") {
				auto track = std::dynamic_pointer_cast<Track>(stash->parseMediaItem(currentItemJson));
				if(track) {
					currentItem = track;
				}
			}
			// update queue
			LinkedList<$<QueueItem>> queueNextItems;
			for(auto itemJson : queueJson.array_items()) {
				queueNextItems.pushBack(QueueItem::fromJson(itemJson, stash));
			}
			LinkedList<$<QueueItem>> queuePastItems;
			for(auto itemJson : queuePastJson.array_items()) {
				queuePastItems.pushBack(QueueItem::fromJson(itemJson, stash));
			}
			if(currentItem.hasValue()) {
				// if current item is queue item, add to past items
				if(auto queueItemPtr = std::get_if<$<QueueItem>>(&currentItem.value())) {
					queuePastItems.pushBack(*queueItemPtr);
				}
			}
			this->queue = PlaybackQueue{
				.pastItems = queuePastItems,
				.items = queueNextItems
			};
			// update context / current item
			if(context && contextItem) {
				this->updateMainContext(context, contextItem, shuffling);
			} else {
				this->applyingItem = currentItem;
				this->updateMainContext(nullptr, nullptr, shuffling);
			}
			return true;
		});
	}



	void PlaybackOrganizer::addEventListener(EventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		listeners.pushBack(listener);
	}

	void PlaybackOrganizer::removeEventListener(EventListener* listener) {
		std::unique_lock<std::mutex> lock(listenersMutex);
		listeners.removeLastEqual(listener);
	}



	Promise<bool> PlaybackOrganizer::previous() {
		auto valid = fgl::new$<bool>(false);
		w$<PlaybackOrganizer> weakSelf = shared_from_this();
		return setPlayingItem([=]() -> Promise<Optional<PlayerItem>> {
			auto self = weakSelf.lock();
			if(!self) {
				return resolveWith(std::nullopt);
			}
			return self->getValidPreviousItem().map([=](auto item) {
				if(item.hasValue()) {
					*valid = true;
				}
				return item;
			});
		}, {
			.trigger = SetPlayingOptions::Trigger::PREVIOUS
		}).map([=]() -> bool {
			return *valid;
		});
	}

	Promise<bool> PlaybackOrganizer::next() {
		auto valid = fgl::new$<bool>(false);
		w$<PlaybackOrganizer> weakSelf = shared_from_this();
		return setPlayingItem([=]() -> Promise<Optional<PlayerItem>> {
			auto self = weakSelf.lock();
			if(!self) {
				return resolveWith(std::nullopt);
			}
			return self->getValidNextItem().map([=](auto item) {
				if(item.hasValue()) {
					*valid = true;
				}
				return item;
			});
		}, {
			.trigger = SetPlayingOptions::Trigger::NEXT
		}).map([=]() -> bool {
			return *valid;
		});
	}

	Promise<void> PlaybackOrganizer::prepareNextIfNeeded() {
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "prepare",
			.cancelMatchingTags = true
		};
		w$<PlaybackOrganizer> weakSelf = shared_from_this();
		return continuousPlayQueue.runSingle(runOptions, coLambda([=](auto task) -> Generator<void> {
			co_yield setGenResumeQueue(DispatchQueue::main());
			co_yield initialGenNext();
			auto self = weakSelf.lock();
			if(!self) {
				co_return;
			}
			if(self->preparedNext) {
				co_return;
			}
			auto maybeNextItem = co_await self->getNextItem();
			co_yield {};
			if(!maybeNextItem.hasValue()) {
				co_return;
			}
			auto nextItem = maybeNextItem.value();
			if(self->delegate == nullptr) {
				self->preparedNext = true;
			} else {
				co_await self->delegate->onPlaybackOrganizerPrepareItem(self, nextItem);
				self->preparedNext = true;
			}
		})).promise;
	}
	
	Promise<void> PlaybackOrganizer::play($<QueueItem> item) {
		FGL_ASSERT(item != nullptr, "item cannot be null");
		return setPlayingItem([=]() -> Promise<Optional<PlayerItem>> {
			return resolveWith(item);
		}, {
			.trigger = SetPlayingOptions::Trigger::PLAY
		});
	}

	Promise<void> PlaybackOrganizer::play($<TrackCollectionItem> item) {
		FGL_ASSERT(item != nullptr, "item cannot be null");
		auto context = item->context().lock();
		FGL_ASSERT(context != nullptr, "item cannot be detached from context");
		return setPlayingItem([context,item]() -> Promise<Optional<PlayerItem>> {
			return resolveWith(item);
		}, {
			.trigger = SetPlayingOptions::Trigger::PLAY
		});
	}

	Promise<void> PlaybackOrganizer::play($<Track> track) {
		FGL_ASSERT(track != nullptr, "track cannot be null");
		return setPlayingItem([=]() -> Promise<Optional<PlayerItem>> {
			return resolveWith(track);
		}, {
			.trigger = SetPlayingOptions::Trigger::PLAY
		});
	}

	Promise<void> PlaybackOrganizer::play(PlayerItem item) {
		if(item.isTrack()) {
			return play(item.asTrack());
		} else if(item.isCollectionItem()) {
			return play(item.asCollectionItem());
		} else if(item.isQueueItem()) {
			return play(item.asQueueItem());
		} else {
			throw std::logic_error("Unhandled state for ItemVariant");
		}
	}

	Promise<void> PlaybackOrganizer::stop() {
		playQueue.cancelAllTasks();
		continuousPlayQueue.cancelAllTasks();
		return Promise<void>::all({ playQueue.waitForCurrentTasks(), continuousPlayQueue.waitForCurrentTasks() });
	}



	$<QueueItem> PlaybackOrganizer::addToQueue($<Track> track) {
		auto queueItem = queue.appendItem(track);
		// emit queue change event
		std::unique_lock<std::mutex> lock(listenersMutex);
		auto listeners = this->listeners;
		lock.unlock();
		auto self = shared_from_this();
		for(auto listener : listeners) {
			listener->onPlaybackOrganizerQueueChange(self);
		}
		return queueItem;
	}

	$<QueueItem> PlaybackOrganizer::addToQueueFront($<Track> track) {
		auto queueItem = queue.prependItem(track);
		// emit queue change event
		std::unique_lock<std::mutex> lock(listenersMutex);
		auto listeners = this->listeners;
		lock.unlock();
		auto self = shared_from_this();
		for(auto listener : listeners) {
			listener->onPlaybackOrganizerQueueChange(self);
		}
		return queueItem;
	}

	$<QueueItem> PlaybackOrganizer::addToQueueRandomly($<Track> track) {
		auto queueItem = queue.insertItemRandomly(track);
		// emit queue change event
		std::unique_lock<std::mutex> lock(listenersMutex);
		auto listeners = this->listeners;
		lock.unlock();
		auto self = shared_from_this();
		for(auto listener : listeners) {
			listener->onPlaybackOrganizerQueueChange(self);
		}
		return queueItem;
	}

	bool PlaybackOrganizer::removeFromQueue($<QueueItem> item) {
		if(queue.removeItem(item)) {
			// emit queue change event
			std::unique_lock<std::mutex> lock(listenersMutex);
			auto listeners = this->listeners;
			lock.unlock();
			auto self = shared_from_this();
			for(auto listener : listeners) {
				listener->onPlaybackOrganizerQueueChange(self);
			}
			return true;
		}
		return false;
	}



	Optional<PlayerItem> PlaybackOrganizer::getCurrentItem() const {
		if(applyingItem.hasValue()) {
			return applyingItem.value();
		} else if(playingItem.hasValue()) {
			return playingItem.value();
		}
		return std::nullopt;
	}

	Promise<Optional<PlayerItem>> PlaybackOrganizer::getPreviousItem() {
		auto queueItem = getPreviousInQueue();
		if(queueItem) {
			return resolveWith(queueItem);
		}
		w$<PlaybackOrganizer> weakSelf = shared_from_this();
		return getPreviousInContext().map(nullptr, [=]($<TrackCollectionItem> item) -> Optional<PlayerItem> {
			if(item) {
				return item;
			}
			auto self = weakSelf.lock();
			if(!self) {
				return std::nullopt;
			}
			auto queueItem = self->getPreviousInQueue();
			if(queueItem) {
				return queueItem;
			}
			return std::nullopt;
		});
	}

	Promise<Optional<PlayerItem>> PlaybackOrganizer::getNextItem() {
		auto queueItem = getNextInQueue();
		if(queueItem) {
			return resolveWith(queueItem);
		}
		w$<PlaybackOrganizer> weakSelf = shared_from_this();
		return getNextInContext().map([=]($<TrackCollectionItem> item) -> Optional<PlayerItem> {
			if(item) {
				return item;
			}
			auto self = weakSelf.lock();
			if(!self) {
				return std::nullopt;
			}
			auto queueItem = self->getNextInQueue();
			if(queueItem) {
				return queueItem;
			}
			return std::nullopt;
		});
	}

	Promise<Optional<PlayerItem>> PlaybackOrganizer::getValidPreviousItem() {
		auto currentContext = this->context;
		w$<PlaybackOrganizer> weakSelf = shared_from_this();
		return getPreviousItem().then([=](auto item) -> Promise<Optional<PlayerItem>> {
			auto self = weakSelf.lock();
			if(!self) {
				return resolveWith(std::nullopt);
			}
			if(self->context != currentContext) {
				return getValidPreviousItem();
			}
			return resolveWith(item);
		});
	}

	Promise<Optional<PlayerItem>> PlaybackOrganizer::getValidNextItem() {
		auto currentContext = this->context;
		w$<PlaybackOrganizer> weakSelf = shared_from_this();
		return getNextItem().then([=](auto item) -> Promise<Optional<PlayerItem>> {
			auto self = weakSelf.lock();
			if(!self) {
				return resolveWith(std::nullopt);
			}
			if(self->context != currentContext) {
				return getValidNextItem();
			}
			return resolveWith(item);
		});
	}



	$<Track> PlaybackOrganizer::getCurrentTrack() const {
		auto item = getCurrentItem();
		if(!item.hasValue()) {
			return nullptr;
		}
		return item->track();
	}

	Promise<$<Track>> PlaybackOrganizer::getPreviousTrack() {
		return getPreviousItem().map(nullptr, [](auto item) -> $<Track> {
			return item ? item->track() : nullptr;
		});
	}

	Promise<$<Track>> PlaybackOrganizer::getNextTrack() {
		return getNextItem().map(nullptr, [](auto item) -> $<Track> {
			return item ? item->track() : nullptr;
		});
	}



	$<QueueItem> PlaybackOrganizer::getPreviousInQueue() const {
		if(queue.pastItems.size() == 0) {
			return nullptr;
		}
		auto currentItem = getCurrentItem();
		if(currentItem.hasValue()) {
			if(auto queueItemPtr = std::get_if<$<QueueItem>>(&currentItem.value());
			   queueItemPtr != nullptr && queue.pastItems.size() > 0 && *queueItemPtr == queue.pastItems.back()) {
				// current item is the last item in queue.pastItems, so get the item before that
				if(queue.pastItems.size() > 1) {
					return *std::prev(queue.pastItems.end(), 2);
				}
				return nullptr;
			}
		}
		return queue.pastItems.back();
	}

	$<QueueItem> PlaybackOrganizer::getNextInQueue() const {
		if(queue.items.size() == 0) {
			return nullptr;
		}
		return queue.items.front();
	}

	Promise<$<TrackCollectionItem>> PlaybackOrganizer::getPreviousInContext() {
		auto prevIndex = getPreviousContextIndex();
		if(!prevIndex) {
			return resolveWith(nullptr);
		}
		return context->getItem(prevIndex.value(), {
			.trackIndexChanges = true
		});
	}

	Promise<$<TrackCollectionItem>> PlaybackOrganizer::getNextInContext() {
		auto nextIndex = getNextContextIndex();
		if(!nextIndex) {
			return resolveWith(nullptr);
		}
		return context->getItem(nextIndex.value(), {
			.trackIndexChanges=true
		});
	}



	$<TrackCollection> PlaybackOrganizer::getContext() const {
		return context;
	}

	Optional<size_t> PlaybackOrganizer::getContextIndex() const {
		if(auto contextIndex = (contextItem ? contextItem->indexInContext() : std::nullopt)) {
			return contextIndex;
		}
		if(std::dynamic_pointer_cast<ShuffledTrackCollection>(context) != nullptr) {
			if(shuffledContextIndex) {
				return shuffledContextIndex->index;
			}
		} else {
			if(sourceContextIndex) {
				return sourceContextIndex->index;
			}
		}
		return std::nullopt;
	}

	Optional<size_t> PlaybackOrganizer::getPreviousContextIndex() const {
		if(!context) {
			return std::nullopt;
		}
		bool wasRemoved = false;
		Optional<size_t> contextIndex;
		if(contextItem) {
			contextIndex = contextItem->indexInContext();
		}
		if(!contextIndex) {
			if(std::dynamic_pointer_cast<ShuffledTrackCollection>(context) != nullptr) {
				if(shuffledContextIndex) {
					wasRemoved = (shuffledContextIndex->state == TrackCollection::ItemIndexMarkerState::REMOVED);
					contextIndex = shuffledContextIndex->index;
				}
			} else {
				if(sourceContextIndex) {
					wasRemoved = (sourceContextIndex->state == TrackCollection::ItemIndexMarkerState::REMOVED);
					contextIndex = sourceContextIndex->index;
				}
			}
		}
		if(!contextIndex) {
			return std::nullopt;
		}
		auto currentItem = getCurrentItem();
		if(auto contextItemPtr = std::get_if<$<TrackCollectionItem>>(&currentItem.value());
		   contextItemPtr != nullptr && *contextItemPtr == contextItem) {
			// current item is context item
			if(contextIndex == 0) {
				return std::nullopt;
			}
			return contextIndex.value() - 1;
		} else {
			// current item is not context item
			if(wasRemoved) {
				if(contextIndex == 0) {
					return std::nullopt;
				}
				return contextIndex.value() - 1;
			} else {
				return contextIndex;
			}
		}
	}

	Optional<size_t> PlaybackOrganizer::getNextContextIndex() const {
		if(!context) {
			return std::nullopt;
		}
		auto contextIndex = contextItem ? contextItem->indexInContext() : std::nullopt;
		if(contextIndex) {
			return contextIndex.value() + 1;
		}
		if(std::dynamic_pointer_cast<ShuffledTrackCollection>(context) != nullptr) {
			if(shuffledContextIndex) {
				if(shuffledContextIndex->state == TrackCollection::ItemIndexMarkerState::REMOVED) {
					return shuffledContextIndex->index;
				} else {
					return shuffledContextIndex->index + 1;
				}
			} else {
				return std::nullopt;
			}
		} else {
			if(sourceContextIndex) {
				if(sourceContextIndex->state == TrackCollection::ItemIndexMarkerState::REMOVED) {
					return sourceContextIndex->index;
				} else {
					return sourceContextIndex->index + 1;
				}
			} else {
				return std::nullopt;
			}
		}
	}

	ArrayList<$<QueueItem>> PlaybackOrganizer::getQueuePastItems() const {
		if(queue.pastItems.size() == 0) {
			return {};
		}
		auto currentItem = getCurrentItem();
		if(currentItem.hasValue()) {
			if(auto queueItemPtr = std::get_if<$<QueueItem>>(&currentItem.value());
			   queueItemPtr != nullptr && *queueItemPtr == queue.pastItems.back()) {
				return ArrayList<$<QueueItem>>(queue.pastItems.begin(), std::prev(queue.pastItems.end(), 1));
			}
		}
		return queue.pastItems;
	}

	ArrayList<$<QueueItem>> PlaybackOrganizer::getQueueItems() const {
		return queue.items;
	}

	$<QueueItem> PlaybackOrganizer::getQueuePastItem(size_t index) const {
		if(index >= queue.pastItems.size()) {
			throw std::invalid_argument("invalid queue past index "+std::to_string(index));
		}
		return *std::next(queue.pastItems.begin(), index);
	}

	$<QueueItem> PlaybackOrganizer::getQueueItem(size_t index) const {
		if(index >= queue.items.size()) {
			throw std::invalid_argument("invalid queue index "+std::to_string(index));
		}
		return *std::next(queue.items.begin(), index);
	}

	size_t PlaybackOrganizer::getQueuePastItemCount() const {
		size_t count = queue.pastItems.size();
		if(count > 0) {
			auto currentItem = getCurrentItem();
			if(currentItem.hasValue()) {
				if(auto queueItemPtr = std::get_if<$<QueueItem>>(&currentItem.value());
				   queueItemPtr != nullptr && *queueItemPtr == queue.pastItems.back()) {
					count -= 1;
				}
			}
		}
		return count;
	}

	size_t PlaybackOrganizer::getQueueItemCount() const {
		return queue.items.size();
	}



	void PlaybackOrganizer::setShuffling(bool shuffling) {
		updateMainContext(context, contextItem, shuffling);
	}

	bool PlaybackOrganizer::isShuffling() const {
		return shuffling;
	}



	bool PlaybackOrganizer::isPreparing() const {
		return continuousPlayQueue.hasTaskWithTag("prepare");
	}

	bool PlaybackOrganizer::hasPreparedNext() const {
		return preparedNext;
	}



	Promise<void> PlaybackOrganizer::prepareCollectionTracks($<TrackCollection> collection, size_t index) {
		size_t startIndex = index;
		if(startIndex <= prefs.contextLoadBuffer) {
			startIndex = 0;
		}
		size_t endIndex = index + prefs.contextLoadBuffer;
		return collection->loadItems(startIndex, (endIndex - startIndex));
	}

	Promise<void> PlaybackOrganizer::setPlayingItem(Function<Promise<Optional<PlayerItem>>()> itemGetter, SetPlayingOptions options) {
		w$<PlaybackOrganizer> weakSelf = shared_from_this();
		return playQueue.run({.cancelAll=true}, [=](auto task) {
			return itemGetter().then([=](auto maybeItem) {
				auto self = weakSelf.lock();
				if(!self) {
					return;
				}
				if(!maybeItem.hasValue()) {
					return;
				}
				auto item = maybeItem.value();
				
				auto oldItem = self->getCurrentItem();
				auto oldContext = self->context;
				if(auto oldShuffledContext = std::dynamic_pointer_cast<ShuffledTrackCollection>(oldContext)) {
					oldContext = oldShuffledContext->source();
				}
				
				bool queueChanged = false;
				if(auto queueItemPtr = std::get_if<$<QueueItem>>(&item)) {
					auto queueItem = *queueItemPtr;
					// check if we've backed out of a context into the past queue
					if(oldItem.hasValue() && std::get_if<$<TrackCollectionItem>>(&oldItem.value())) {
						auto oldCollectionItem = std::get<$<TrackCollectionItem>>(oldItem.value());
						if(oldCollectionItem && oldCollectionItem->context().lock() == oldContext
						   && queue.isPastItem(queueItem) && options.trigger != SetPlayingOptions::Trigger::NEXT) {
							// move to previous context item or set index state to "removed"
							auto prevItemPromise = getPreviousInContext();
							auto setContextIndexRemoved = [&]() {
								if(sourceContextIndex) {
									sourceContextIndex->state = TrackCollection::ItemIndexMarkerState::REMOVED;
								}
								if(shuffledContextIndex) {
									shuffledContextIndex->state = TrackCollection::ItemIndexMarkerState::REMOVED;
								}
								self->contextItem = nullptr;
							};
							if(prevItemPromise.isComplete()) {
								auto prevItem = maybeTry([&](){ return prevItemPromise.get(); }, nullptr);
								if(!prevItem) {
									setContextIndexRemoved();
								} else {
									updateMainContext(oldContext, prevItem, self->shuffling);
								}
							} else {
								setContextIndexRemoved();
							}
						}
					}
					// update queue
					queue.shiftToItem(queueItem);
					if(!prefs.pastQueueEnabled) {
						queue.clearPastItems();
					}
					queueChanged = true;
				}
				else if(auto collectionItemPtr = std::get_if<$<TrackCollectionItem>>(&item)) {
					auto collectionItem = *collectionItemPtr;
					auto collection = collectionItem->context().lock();
					if(!collection) {
						throw std::logic_error("TrackCollectionItem has null context");
					}
					self->updateMainContext(collection, collectionItem, self->shuffling);
					// update item
					auto contextItem = self->contextItem;
					FGL_ASSERT(contextItem != nullptr, "contextItem should not be null after call to updateMainContext");
					item = contextItem;
				}
				else if(auto trackPtr = std::get_if<$<Track>>(&item)) {
					self->updateMainContext(nullptr, nullptr, self->shuffling);
				}
				
				auto newContext = self->context;
				if(auto newShuffledContext = std::dynamic_pointer_cast<ShuffledTrackCollection>(newContext)) {
					newContext = newShuffledContext->source();
				}
				
				// handle specific trigger cases
				if(options.trigger == SetPlayingOptions::Trigger::NEXT) {
					// clear past queue if queue exited
					if(oldItem.hasValue() && std::get_if<$<QueueItem>>(&oldItem.value())) {
						// previous item was queue item
						if(!std::get_if<$<QueueItem>>(&item) && queue.items.empty()) {
							// new item is not queue item and we have no more queue items
							if(prefs.clearPastQueueOnExitQueue && !queue.pastItems.empty()) {
								queue.clearPastItems();
								queueChanged = true;
							}
						}
					}
					else if(!std::get_if<$<QueueItem>>(&item) && queue.items.empty()) {
						// previous item and current item are not queue items, and we have no more queue items
						if(prefs.clearPastQueueOnNextAfterExitQueue && !queue.pastItems.empty()) {
							queue.clearPastItems();
							queueChanged = true;
						}
					}
				}
				else if(options.trigger == SetPlayingOptions::Trigger::PREVIOUS) {
					// shift queue if we've backed out of it
					if(oldItem.hasValue()) {
						if(auto queueItemPtr = std::get_if<$<QueueItem>>(&oldItem.value());
						   queueItemPtr != nullptr && !std::get_if<$<QueueItem>>(&item)) {
							// previous item was queue item, new item is not
							auto queueItem = *queueItemPtr;
							if(queue.shiftBehindItem(queueItem)) {
								queueChanged = true;
							}
						}
					}
				}
				else if(options.trigger == SetPlayingOptions::Trigger::PLAY) {
					// clear past queue if context changed and we had an item previously
					if(!std::get_if<$<QueueItem>>(&item) && oldItem.hasValue() && oldContext != newContext) {
						if(prefs.clearPastQueueOnContextChange && !queue.pastItems.empty()) {
							queue.clearPastItems();
							queueChanged = true;
						}
					}
				}
				
				// if old item was queue item and new item is not, ensure a queue changed event
				if(!std::get_if<$<QueueItem>>(&item) && oldItem.hasValue() && std::get_if<$<QueueItem>>(&oldItem.value())) {
					queueChanged = true;
				}
				
				// apply item
				applyingItem = item;
				applyPlayingItem(item);
				{// emit item change event
					std::unique_lock<std::mutex> lock(self->listenersMutex);
					auto listeners = self->listeners;
					lock.unlock();
					for(auto listener : listeners) {
						listener->onPlaybackOrganizerItemChange(self);
					}
				}
				if(queueChanged) {
					// emit queue change event
					std::unique_lock<std::mutex> lock(self->listenersMutex);
					auto listeners = self->listeners;
					lock.unlock();
					for(auto listener : listeners) {
						listener->onPlaybackOrganizerQueueChange(self);
					}
				}
			});
		}).promise;
	}

	Promise<void> PlaybackOrganizer::applyPlayingItem(PlayerItem item) {
		auto runOptions = AsyncQueue::RunOptions{
			.tag = "play",
			.cancelTags = { "prepare", "play" }
		};
		w$<PlaybackOrganizer> weakSelf = shared_from_this();
		return continuousPlayQueue.run(runOptions, coLambda([=](auto task) -> Generator<void> {
			co_yield setGenResumeQueue(DispatchQueue::main());
			co_yield initialGenNext();
			auto self = weakSelf.lock();
			if(!self) {
				co_return;
			}
			auto track = item.track();
			while(true) {
				try {
					// TODO some items may have a null track and need to load it in the future possibly?
					if(track != nullptr && track->needsData()) {
						co_await track->fetchDataIfNeeded();
					}
					if(track != nullptr && track->isPlayable()) {
						// play track
						if(self->delegate != nullptr) {
							co_await self->delegate->onPlaybackOrganizerPlayItem(self, item);
						}
						// apply playing item
						self->playingItem = item;
						self->applyingItem = std::nullopt;
						self->preparedNext = false;
						co_yield {};
						// load surrounding context until success
						while(true) {
							try {
								auto context = self->context;
								auto contextIndex = self->getContextIndex();
								if(!context || !contextIndex) {
									break;
								}
								co_await self->prepareCollectionTracks(context, contextIndex.value());
								break;
							}
							catch(...) {
								console::error("Failed to load next tracks in context: ", utils::getExceptionDetails(std::current_exception()).fullDescription);
								// ignore error and try again
							}
							co_yield {};
							for(size_t i=0; i<10; i++) {
								std::this_thread::sleep_for(std::chrono::milliseconds(100));
								co_yield {};
							}
						}
					}
					else {
						// load next track until success
						while(true) {
							try {
								co_await self->next();
								break;
							}
							catch(...) {
								console::error("Failed to skip to next track after ",
									   track->name().c_str(), ": ", utils::getExceptionDetails(std::current_exception()).fullDescription);
								// ignore error and try again
							}
							co_yield {};
							for(size_t i=0; i<4; i++) {
								std::this_thread::sleep_for(std::chrono::milliseconds(100));
								co_yield {};
							}
						}
					}
					co_return;
				}
				catch(...) {
					console::error("Failed to play track ",
						   track->name().c_str(), ": ", utils::getExceptionDetails(std::current_exception()).fullDescription);
					// ignore error and try again
				}
				co_yield {};
				for(size_t i=0; i<1; i++) {
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					co_yield {};
				}
			}
		})).promise;
	}

	void PlaybackOrganizer::updateMainContext($<TrackCollection> context, $<TrackCollectionItem> contextItem, bool shuffling) {
		FGL_ASSERT(context || !contextItem, "if context is null, contextItem should also be null");
		auto prevContext = this->context;
		auto prevContextItem = this->contextItem;
		$<TrackCollection> prevSourceContext;
		$<TrackCollectionItem> prevSourceContextItem;
		$<ShuffledTrackCollection> prevShuffledContext;
		$<ShuffledTrackCollectionItem> prevShuffledContextItem;
		auto prevSourceContextIndex = this->sourceContextIndex;
		auto prevShuffledContextIndex = this->shuffledContextIndex;
		if(prevContext) {
			if(auto shuffledContext = std::dynamic_pointer_cast<ShuffledTrackCollection>(prevContext)) {
				auto shuffledContextItem = std::static_pointer_cast<ShuffledTrackCollectionItem>(prevContextItem);
				prevSourceContext = shuffledContext->source();
				prevSourceContextItem = shuffledContextItem ? shuffledContextItem->sourceItem() : nullptr;
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
		TrackCollection::ItemIndexMarker sourceContextIndex;
		TrackCollection::ItemIndexMarker shuffledContextIndex;
		
		if(context) {
			// TODO lock context
			if(shuffling) {
				if(auto castShuffledContext = std::dynamic_pointer_cast<ShuffledTrackCollection>(context)) {
					auto castShuffledItem = std::dynamic_pointer_cast<ShuffledTrackCollectionItem>(contextItem);
					sourceContext = castShuffledContext->source();
					sourceContextItem = castShuffledItem ? castShuffledItem->sourceItem() : nullptr;
					shuffledContext = castShuffledContext;
					shuffledContextItem = castShuffledItem;
				}
				else {
					sourceContext = context;
					sourceContextItem = contextItem;
					ArrayList<$<TrackCollectionItem>> sourceItems;
					if(sourceContextItem) {
						sourceItems.pushBack(sourceContextItem);
					}
					shuffledContext = ShuffledTrackCollection::new$(sourceContext, sourceItems);
					shuffledContextItem = std::static_pointer_cast<ShuffledTrackCollectionItem>(shuffledContext->itemAt(0));
					context = shuffledContext;
					contextItem = shuffledContextItem;
				}
				FGL_ASSERT(prevSourceContext == sourceContext || (sourceContextItem && sourceContextItem->context().lock() == sourceContext), "A valid context item must be supplied if the source context has changed");
				FGL_ASSERT(contextItem == nullptr || contextItem->context().lock() == context, "contextItem must belong to context");
				if(prevSourceContext == sourceContext && prevSourceContextItem == sourceContextItem) {
					sourceContextIndex = prevSourceContextIndex;
				} else if(auto sourceIndex = (sourceContextItem ? sourceContextItem->indexInContext() : std::nullopt)) {
					sourceContextIndex = sourceContext->watchIndex(sourceIndex.value());
				}
				if(prevShuffledContext == shuffledContext && prevShuffledContextItem == shuffledContextItem) {
					shuffledContextIndex = prevShuffledContextIndex;
				} else if(auto shuffledIndex = (shuffledContextItem ? shuffledContextItem->indexInContext() : std::nullopt)) {
					shuffledContextIndex = shuffledContext->watchIndex(shuffledIndex.value());
				} else {
					shuffledContextIndex = shuffledContext->watchRemovedIndex(0);
				}
				contextIndex = shuffledContextIndex ? maybe(shuffledContextIndex->index) : std::nullopt;
			}
			else {
				if(auto castShuffledContext = std::dynamic_pointer_cast<ShuffledTrackCollection>(context)) {
					auto castShuffledItem = std::dynamic_pointer_cast<ShuffledTrackCollectionItem>(contextItem);
					context = castShuffledContext->source();
					contextItem = castShuffledItem ? castShuffledItem->sourceItem() : nullptr;
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
				FGL_ASSERT(prevSourceContext == sourceContext || (sourceContextItem && sourceContextItem->context().lock() == sourceContext), "A valid context item must be supplied if the source context has changed");
				FGL_ASSERT(contextItem == nullptr || contextItem->context().lock() == context, "contextItem must belong to context");
				if(prevSourceContext == sourceContext && prevSourceContextItem == sourceContextItem) {
					sourceContextIndex = prevSourceContextIndex;
				} else if(auto sourceIndex = (sourceContextItem ? sourceContextItem->indexInContext() : std::nullopt)) {
					sourceContextIndex = sourceContext->watchIndex(sourceIndex.value());
				}
				contextIndex = sourceContextIndex ? maybe(sourceContextIndex->index) : std::nullopt;
			}
		}
		
		if(sourceContext != prevSourceContext) {
			// TODO subscribe / unsubscribe from context changes
		}
		if(shuffledContext != prevShuffledContext) {
			// TODO subscribe / unsubscribe from context changes
		}
		
		if(context && context != this->context) {
			// TODO log error in logger
			prepareCollectionTracks(context, contextIndex.valueOr(0)).except([=](std::exception_ptr error) {
				console::error("Error preparing collection tracks: ", utils::getExceptionDetails(error).fullDescription);
			});
		}
		
		this->context = context;
		this->contextItem = contextItem;
		this->sourceContextIndex = sourceContextIndex;
		this->shuffledContextIndex = shuffledContextIndex;
		this->shuffling = shuffling;
		if(!this->playingItem.hasValue() && !this->applyingItem.hasValue() && contextItem) {
			this->applyingItem = contextItem;
		}
		else if(sourceContextItem && prevSourceContextItem && prevSourceContextItem == sourceContextItem) {
			if(this->applyingItem.hasValue()
			   && (this->applyingItem == PlayerItem(prevSourceContextItem) || this->applyingItem == PlayerItem(prevShuffledContextItem))) {
				this->applyingItem = contextItem;
			}
			else if(this->playingItem.hasValue()
			   && (this->playingItem == PlayerItem(prevSourceContextItem) || this->playingItem == PlayerItem(prevShuffledContextItem))) {
				this->playingItem = contextItem;
			}
		}
		
		// unwatch previous indexes
		if(prevSourceContextIndex && prevSourceContextIndex != sourceContextIndex) {
			FGL_ASSERT(prevSourceContext != nullptr, "if there is a previous sourceContextIndex, there has to be a previous sourceContext");
			prevSourceContext->unwatchIndex(prevSourceContextIndex);
		}
		if(prevShuffledContextIndex && prevShuffledContextIndex != shuffledContextIndex) {
			FGL_ASSERT(prevShuffledContext != nullptr, "if there is a previous shuffledContextIndex, there has to be a previous shuffledContext");
			prevShuffledContext->unwatchIndex(prevShuffledContextIndex);
		}
	}
}

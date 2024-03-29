//
//  MediaLibrary.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 6/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "MediaLibrary.hpp"
#include "MediaLibraryProxyProvider.hpp"
#include <soundhole/providers/soundhole/SoundHoleMediaProvider.hpp>
#include <soundhole/utils/Utils.hpp>
#include <soundhole/utils/js/JSError.hpp>

namespace sh {
	MediaLibrary::MediaLibrary(Options options)
	: libraryProxyProvider(nullptr), db(nullptr), mediaProviderStash(options.mediaProviderStash) {
		libraryProxyProvider = new MediaLibraryProxyProvider(this);
		db = new MediaDatabase({
			.path = options.dbPath,
			.mediaProviderStash = options.mediaProviderStash,
			.scrobblerStash = options.scrobblerStash
		});
	}

	MediaLibrary::~MediaLibrary() {
		delete db;
		delete libraryProxyProvider;
	}



	#pragma mark Initialization

	Promise<void> MediaLibrary::initialize() {
		return Promise<void>::resolve().then([=]() {
			db->open();
			return db->initialize();
		});
	}



	#pragma mark Database

	Promise<void> MediaLibrary::resetDatabase() {
		return db->reset();
	}

	MediaDatabase* MediaLibrary::database() {
		return db;
	}

	const MediaDatabase* MediaLibrary::database() const {
		return db;
	}



	#pragma mark Library Proxy Provider

	MediaLibraryProxyProvider* MediaLibrary::proxyProvider() {
		return libraryProxyProvider;
	}

	const MediaLibraryProxyProvider* MediaLibrary::proxyProvider() const {
		return libraryProxyProvider;
	}



	#pragma mark Synchronization
	
	bool MediaLibrary::isSynchronizingLibrary(const String& libraryProviderName) {
		auto taskNode = getSynchronizeLibraryTask(libraryProviderName);
		if(taskNode) {
			return true;
		}
		return false;
	}

	bool MediaLibrary::isSynchronizingLibraries() {
		for(MediaProvider* provider : mediaProviderStash->getMediaProviders()) {
			if(isSynchronizingLibrary(provider->name())) {
				return true;
			}
		}
		return false;
	}

	Optional<AsyncQueue::TaskNode> MediaLibrary::getSynchronizeLibraryTask(const String& libraryProviderName) {
		return synchronizeQueue.getTaskWithTag("sync:"+libraryProviderName);
	}

	Optional<AsyncQueue::TaskNode> MediaLibrary::getSynchronizeAllLibrariesTask() {
		return synchronizeAllQueue.getTaskWithTag("sync:all");
	}

	AsyncQueue::TaskNode MediaLibrary::synchronizeProviderLibrary(const String& libraryProviderName) {
		auto provider = mediaProviderStash->getMediaProvider(libraryProviderName);
		if(provider == nullptr) {
			throw std::runtime_error("No provider exists with name "+libraryProviderName);
		}
		return synchronizeProviderLibrary(provider);
	}

	AsyncQueue::TaskNode MediaLibrary::synchronizeProviderLibrary(MediaProvider* libraryProvider) {
		auto runOptions = AsyncQueue::RunOptions{
			.tag=("sync:"+libraryProvider->name()),
			.initialStatus = AsyncQueue::Task::Status{
				.text = "Waiting to sync "+libraryProvider->displayName()+" library"
			}
		};
		return synchronizeQueue.runSingle(runOptions, coLambda([=](auto task) -> Generator<void> {
			co_yield setGenResumeQueue(DispatchQueue::main());
			co_yield initialGenNext();
			if(!libraryProvider->isLoggedIn()) {
				throw std::runtime_error(libraryProvider->displayName()+" provider is not logged in");
			}
			if(!libraryProvider->hasLibrary()) {
				throw std::runtime_error(libraryProvider->displayName()+" has no user library");
			}
			task->setStatus({
				.progress = 0,
				.text = "Checking "+libraryProvider->displayName()+" library sync state"
			});
			// begin generator
			auto syncResumeDBKey = db->stateKey_syncLibraryResumeData(libraryProvider);
			auto syncResumeStr = co_await db->getStateValue(syncResumeDBKey, "");
			Json syncResumeData;
			if(syncResumeStr.length() > 0) {
				std::string syncResumeError;
				syncResumeData = Json::parse(syncResumeStr, syncResumeError);
				if(syncResumeError.length() > 0) {
					syncResumeData = Json();
				}
			}
			co_yield {};
			// sync provider library
			auto libraryGenerator = libraryProvider->generateLibrary({.resumeData=syncResumeData});
			while(true) {
				// get library items from provider until success
				task->setStatusText("Synchronizing "+libraryProvider->displayName()+" library");
				MediaProvider::LibraryItemGenerator::YieldResult yieldResult;
				bool failed = false;
				Optional<double> retryAfter;
				try {
					yieldResult = co_await libraryGenerator.next();
				} catch(Error& error) {
					task->setStatusText("Error: "+error.toString());
					failed = true;
					retryAfter = error.getDetail("retryAfter").maybeAs<double>();
				} catch(std::exception& error) {
					task->setStatusText((String)"Error: "+error.what());
					failed = true;
				}
				// handle error
				if(failed) {
					co_yield {};
					// TODO split up await time so it yields multiple times
					co_await resumeAfter(std::chrono::milliseconds((long)(retryAfter.valueOr(2.0) * 1000)));
					co_yield {};
					continue;
				}
				// handle yield result
				if(yieldResult.value) {
					// cache library items
					double prevProgress = task->getStatus().progress;
					double progressDiff = yieldResult.value->progress - prevProgress;
					double itemProgressDiff = progressDiff / (double)yieldResult.value->items.size();
					size_t libraryItemIndex = 0;
					for(auto libraryItem : yieldResult.value->items) {
						// get index and start progress
						size_t index = libraryItemIndex;
						double itemProgressStart = prevProgress + (itemProgressDiff * (double)index);
						// fetch missing data if needed
						if(libraryItem.mediaItem->needsData()) {
							task->setStatusText("Fetching "+libraryProvider->displayName()+" "+libraryItem.mediaItem->type()+" "+libraryItem.mediaItem->name());
							task->setStatusProgress(itemProgressStart);
							co_await libraryItem.mediaItem->fetchDataIfNeeded();
							task->setStatusText("Synchronizing "+libraryProvider->displayName()+" library");
						}
						// increment index
						libraryItemIndex += 1;
						// fetch tracks if item is a collection
						if(auto collection = std::dynamic_pointer_cast<TrackCollection>(libraryItem.mediaItem)) {
							// check if version ID has changed
							auto existingVersionId = (co_await db->getTrackCollectionJson(collection->uri()).exceptReturn(Json()))["versionId"];
							if(existingVersionId.is_string() && collection->versionId() == existingVersionId.string_value()) {
								continue;
							}
							co_await db->cacheTrackCollections({ collection });
							// cache collection items
							size_t itemsOffset = 0;
							task->setStatusText("Synchronizing "+libraryProvider->displayName()+" "+collection->type()+" "+collection->name());
							auto collectionItemGenerator = collection->generateItems();
							while(true) {
								TrackCollection::ItemGenerator::YieldResult itemsYieldResult;
								failed = false;
								retryAfter = std::nullopt;
								try {
									itemsYieldResult = co_await collectionItemGenerator.next();
								} catch(Error& error) {
									task->setStatusText("Error: "+error.toString());
									failed = true;
									retryAfter = error.getDetail("retryAfter").maybeAs<double>();
								} catch(std::exception& error) {
									task->setStatusText((String)"Error: "+error.what());
									failed = true;
								}
								// handle error
								if(failed) {
									co_yield {};
									// TODO split up await time so it yields multiple times
									co_await resumeAfter(std::chrono::milliseconds((long)(retryAfter.valueOr(2.0) * 1000)));
									co_yield {};
									continue;
								}
								// handle yield result
								if(itemsYieldResult.value) {
									size_t nextOffset = itemsOffset + itemsYieldResult.value->size();
									co_await db->cacheTrackCollectionItems(collection, sql::IndexRange{
										.startIndex = itemsOffset,
										.endIndex = nextOffset
									});
									itemsOffset += itemsYieldResult.value->size();
									if(collection->itemCount()) {
										task->setStatusProgress(itemProgressStart + (((double)nextOffset / (double)collection->itemCount().value()) * itemProgressDiff));
									}
									// TODO reset items to save memory
								}
								if(itemsYieldResult.done) {
									break;
								}
								co_yield {};
								std::this_thread::sleep_for(std::chrono::milliseconds(200));
								co_yield {};
							}
							// update versionId
							co_await db->updateTrackCollectionVersionId(collection);
							// sync library item
							co_await db->cacheLibraryItems({ libraryItem });
						} else {
							task->setStatusProgress(itemProgressStart + itemProgressDiff);
						}
						task->setStatusText("Synchronizing "+libraryProvider->displayName()+" library");
						co_yield {};
					}
					
					// sync library items to database
					auto cacheOptions = MediaDatabase::CacheOptions{
						.dbState = {
							{ syncResumeDBKey, yieldResult.value->resumeData.dump() }
						}
					};
					co_await db->cacheLibraryItems(yieldResult.value->items, cacheOptions);
					task->setStatusProgress(yieldResult.value->progress);
				}
				if(yieldResult.done) {
					break;
				}
				co_yield {};
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				co_yield {};
			}
			
			task->setStatus({
				.progress = 1.0,
				.text = "Finished synchronizing "+libraryProvider->displayName()+" library"
			});
		}));
	}

	AsyncQueue::TaskNode MediaLibrary::synchronizeAllLibraries() {
		auto runOptions = AsyncQueue::RunOptions{
			.tag="sync:all",
			.initialStatus = AsyncQueue::Task::Status{
				.text = "Waiting to sync all libraries"
			}
		};
		return synchronizeAllQueue.runSingle(runOptions, [=](auto task) {
			auto successCount = fgl::new$<size_t>(0);
			auto providers = fgl::new$<LinkedList<MediaProvider*>>();
			
			// get providers to synchronize
			struct QueuedProviderNode {
				size_t index;
				MediaProvider* provider;
			};
			LinkedList<QueuedProviderNode> queuedProviders;
			for(auto provider : mediaProviderStash->getMediaProviders()) {
				if(provider->hasLibrary() && provider->isLoggedIn()) {
					String taskTag = "sync:"+provider->name();
					auto taskNode = synchronizeQueue.getTaskWithTag(taskTag);
					auto taskIndex = synchronizeQueue.indexOfTaskWithTag(taskTag);
					if(taskNode && !taskNode->task->isDone() && !taskNode->task->isCancelled() && taskIndex) {
						queuedProviders.pushBack({ taskIndex.value(), provider });
					} else {
						providers->pushBack(provider);
					}
				}
			}
			queuedProviders.sort([](auto& a, auto& b) {
				return a.index <= b.index;
			});
			providers->insert(providers->begin(), queuedProviders.map([](auto& node) -> MediaProvider* {
				return node.provider;
			}));
			
			size_t providerCount = providers->size();
			return Generator<void,void>([=]() {
				using YieldResult = typename Generator<void,void>::YieldResult;
				if(providers->size() == 0) {
					task->setStatus({
						.progress = 1.0,
						.text = (String)""+(*successCount)+" / "+providerCount+" libraries synced successfully"
					});
					return Promise<YieldResult>::resolve(YieldResult{
						.done=true
					});
				}
				size_t providerIndex = providerCount - providers->size();
				auto provider = providers->extractFront();
				auto taskNode = synchronizeProviderLibrary(provider);
				auto syncTask = taskNode.task;
				auto syncTaskStatus = syncTask->getStatus();
				double progress = ((double)providerIndex + syncTaskStatus.progress) / (double)providerCount;
				if(syncTaskStatus.text.empty()) {
					task->setStatus({
						.progress = progress,
						.text = "Waiting for "+provider->displayName()+" library sync to start"
					});
				} else {
					task->setStatus({
						.progress = progress,
						.text = syncTaskStatus.text
					});
				}
				size_t cancelListenerId = task->addCancelListener([=](auto task) {
					syncTask->cancel();
				});
				size_t statusChangeListenerId = syncTask->addStatusChangeListener([=](auto syncTask, size_t listenerId) {
					auto syncTaskStatus = syncTask->getStatus();
					task->setStatus({
						.progress = ((double)providerIndex + syncTaskStatus.progress) / (double)providerCount,
						.text = syncTaskStatus.text
					});
				});
				return taskNode.promise.then([=]() {
					task->removeCancelListener(cancelListenerId);
					syncTask->removeStatusChangeListener(statusChangeListenerId);
					*successCount += 1;
				}).except([=](std::exception_ptr error) -> Promise<void> {
					task->removeCancelListener(cancelListenerId);
					syncTask->removeStatusChangeListener(statusChangeListenerId);
					task->setStatus({
						.progress = ((double)providerIndex + 1.0) / (double)providerCount,
						.text = (String)"Error syncing "+provider->displayName()+" library: "+utils::getExceptionDetails(error).message
					});
					for(size_t i=0; i<6; i++) {
						if(task->isCancelled()) {
							co_return;
						}
						co_await resumeAfter(std::chrono::milliseconds(500));
					}
				}).map([=]() -> YieldResult {
					return YieldResult{
						.done = false
					};
				});
			});
		});
	}




	#pragma mark Library Tracks

	Promise<$<MediaLibraryTracksCollection>> MediaLibrary::getLibraryTracksCollection(GetLibraryTracksCollectionOptions options) {
		return libraryProxyProvider->getLibraryTracksCollection(options);
	}



	#pragma mark Library Albums

	Promise<LinkedList<$<Album>>> MediaLibrary::getLibraryAlbums(LibraryAlbumsFilters filters) {
		return db->getSavedAlbumsJson({
			.libraryProvider = (filters.libraryProvider != nullptr) ? filters.libraryProvider->name() : String(),
			.orderBy=filters.orderBy,
			.order=filters.order
		}).map(nullptr, [=](MediaDatabase::GetJsonItemsListResult results) -> LinkedList<$<Album>> {
			return results.items.map([=](Json json) -> $<Album> {
				auto mediaItemJson = json["mediaItem"];
				auto providerName = mediaItemJson["provider"];
				if(!providerName.is_string()) {
					throw std::invalid_argument("album provider must be a string");
				}
				auto provider = this->mediaProviderStash->getMediaProvider(providerName.string_value());
				if(provider == nullptr) {
					throw std::invalid_argument("invalid provider name for album: "+providerName.string_value());
				}
				return provider->album(Album::Data::fromJson(mediaItemJson, this->mediaProviderStash));
			});
		});
	}

	MediaLibrary::LibraryAlbumsGenerator MediaLibrary::generateLibraryAlbums(GenerateLibraryAlbumsOptions options) {
		auto offset = fgl::new$<size_t>(options.offset);
		using YieldResult = typename LibraryAlbumsGenerator::YieldResult;
		return LibraryAlbumsGenerator([=]() {
			size_t startIndex = *offset;
			size_t endIndex = (size_t)-1;
			if(options.chunkSize != (size_t)-1) {
				endIndex = startIndex + options.chunkSize;
			}
			return db->getSavedAlbumsJson({
				.libraryProvider = (options.filters.libraryProvider != nullptr) ? options.filters.libraryProvider->name() : String(),
				.range = sql::IndexRange{
					.startIndex = startIndex,
					.endIndex = endIndex
				},
				.orderBy = options.filters.orderBy,
				.order = options.filters.order
			}).map(nullptr, [=](MediaDatabase::GetJsonItemsListResult results) -> YieldResult {
				auto albums = results.items.map([=](Json json) -> $<Album> {
					auto mediaItemJson = json["mediaItem"];
					auto providerName = mediaItemJson["provider"];
					if(!providerName.is_string()) {
						throw std::invalid_argument("album provider must be a string");
					}
					auto provider = this->mediaProviderStash->getMediaProvider(providerName.string_value());
					if(provider == nullptr) {
						throw std::invalid_argument("invalid provider name for album: "+providerName.string_value());
					}
					return provider->album(Album::Data::fromJson(mediaItemJson, this->mediaProviderStash));
				});
				size_t albumsOffset = *offset;
				*offset += options.chunkSize;
				bool done = (*offset >= results.total);
				return YieldResult{
					.value = GenerateLibraryAlbumsResult{
						.albums = albums,
						.offset = albumsOffset,
						.total = results.total
					},
					.done = done
				};
			});
		});
	}



	#pragma mark Library Playlists

	Promise<LinkedList<$<Playlist>>> MediaLibrary::getLibraryPlaylists(LibraryPlaylistsFilters filters) {
		return db->getSavedPlaylistsJson({
			.libraryProvider = (filters.libraryProvider != nullptr) ? filters.libraryProvider->name() : String(),
			.orderBy=filters.orderBy,
			.order=filters.order
		}).map(nullptr, [=](MediaDatabase::GetJsonItemsListResult results) -> LinkedList<$<Playlist>> {
			return results.items.map([=](Json json) -> $<Playlist> {
				auto mediaItemJson = json["mediaItem"];
				auto providerName = mediaItemJson["provider"];
				if(!providerName.is_string()) {
					throw std::invalid_argument("playlist provider must be a string");
				}
				auto provider = this->mediaProviderStash->getMediaProvider(providerName.string_value());
				if(provider == nullptr) {
					throw std::invalid_argument("invalid provider name for playlist: "+providerName.string_value());
				}
				return provider->playlist(Playlist::Data::fromJson(mediaItemJson, this->mediaProviderStash));
			});
		});
	}

	MediaLibrary::LibraryPlaylistsGenerator MediaLibrary::generateLibraryPlaylists(GenerateLibraryPlaylistsOptions options) {
		auto offset = fgl::new$<size_t>(options.offset);
		using YieldResult = typename LibraryPlaylistsGenerator::YieldResult;
		return LibraryPlaylistsGenerator([=]() {
			size_t startIndex = *offset;
			size_t endIndex = (size_t)-1;
			if(options.chunkSize != (size_t)-1) {
				endIndex = startIndex + options.chunkSize;
			}
			return db->getSavedPlaylistsJson({
				.libraryProvider = (options.filters.libraryProvider != nullptr) ? options.filters.libraryProvider->name() : String(),
				.range = sql::IndexRange{
					.startIndex = startIndex,
					.endIndex = endIndex
				},
				.orderBy = options.filters.orderBy,
				.order = options.filters.order
			}).map(nullptr, [=](MediaDatabase::GetJsonItemsListResult results) -> YieldResult {
				auto playlists = results.items.map([=](Json json) -> $<Playlist> {
					auto mediaItemJson = json["mediaItem"];
					auto providerName = mediaItemJson["provider"];
					if(!providerName.is_string()) {
						throw std::invalid_argument("playlist provider must be a string");
					}
					auto provider = this->mediaProviderStash->getMediaProvider(providerName.string_value());
					if(provider == nullptr) {
						throw std::invalid_argument("invalid provider name for playlist: "+providerName.string_value());
					}
					return provider->playlist(Playlist::Data::fromJson(mediaItemJson, this->mediaProviderStash));
				});
				size_t playlistsOffset = *offset;
				*offset += options.chunkSize;
				bool done = (*offset >= results.total);
				return YieldResult{
					.value = GenerateLibraryPlaylistsResult{
						.playlists = playlists,
						.offset = playlistsOffset,
						.total = results.total
					},
					.done = done
				};
			});
		});
	}



	#pragma mark Playback History

	Promise<MediaLibrary::GetPlaybackHistoryItemsResult> MediaLibrary::getPlaybackHistoryItems(GetPlaybackHistoryItemsOptions options) {
		return db->getPlaybackHistoryItemsJson({
			.filters = {
				.provider = options.filters.provider ? options.filters.provider->name() : String(),
				.trackURIs = options.filters.trackURIs,
				.minDate = options.filters.minDate,
				.minDateInclusive = options.filters.minDateInclusive,
				.maxDate = options.filters.maxDate,
				.maxDateInclusive = options.filters.maxDateInclusive,
				.minDuration = options.filters.minDuration,
				.minDurationRatio = options.filters.minDurationRatio,
				.includeNullDuration = options.filters.includeNullDuration,
				.visibility = options.filters.visibility,
				.scrobbledBy = options.filters.scrobbledBy,
				.notScrobbledBy = options.filters.notScrobbledBy
			},
			.range = sql::IndexRange{
				.startIndex = options.offset,
				.endIndex = (options.limit == (size_t)-1) ? (size_t)-1 : (options.offset + options.limit)
			},
			.order = options.filters.order
		}).map(nullptr, [=](auto results) {
			return GetPlaybackHistoryItemsResult{
				.items = results.items.map([=](auto& json) {
					return PlaybackHistoryItem::fromJson(json, mediaProviderStash);
				}),
				.total = results.total
			};
		});
	}

	MediaLibrary::PlaybackHistoryItemGenerator MediaLibrary::generatePlaybackHistoryItems(GeneratePlaybackHistoryItemsOptions options) {
		struct SharedData {
			size_t offset;
			Optional<Date> prevDate;
		};
		auto sharedData = fgl::new$<SharedData>(SharedData{
			.offset = options.offset,
			.prevDate = std::nullopt
		});
		using YieldResult = typename PlaybackHistoryItemGenerator::YieldResult;
		return PlaybackHistoryItemGenerator([=]() -> Promise<YieldResult> {
			auto queryOptions = GetPlaybackHistoryItemsOptions{
				.offset = sharedData->offset,
				.limit = options.chunkSize,
				.filters = options.filters
			};
			// if we're not at the first query, offset by date
			if(sharedData->prevDate) {
				if(options.filters.order == sql::Order::DESC) {
					// update with new max date
					queryOptions.filters.maxDate = sharedData->prevDate.value();
					queryOptions.filters.maxDateInclusive = false;
					queryOptions.offset = 0;
				}
				else if(options.filters.order == sql::Order::ASC) {
					// update with new min date
					queryOptions.filters.minDate = sharedData->prevDate.value();
					queryOptions.filters.minDateInclusive = false;
					queryOptions.offset = 0;
				}
			}
			size_t queryOffset = queryOptions.offset;
			// get items
			return this->getPlaybackHistoryItems(queryOptions).map(nullptr, [=](auto result) -> YieldResult {
				sharedData->offset += result.items.size();
				bool done = false;
				if((queryOffset + result.items.size()) >= result.total) {
					done = true;
				}
				return YieldResult{
					.value = result.items,
					.done = done
				};
			});
		});
	}

	Promise<$<PlaybackHistoryTrackCollection>> MediaLibrary::getPlaybackHistoryCollection(GetPlaybackHistoryCollectionOptions options) {
		return libraryProxyProvider->getPlaybackHistoryCollection(options);
	}



	#pragma mark Creating Media

	Promise<$<Playlist>> MediaLibrary::createPlaylist(String name, MediaProvider* provider, CreatePlaylistOptions options) {
		return provider->createPlaylist(name, options).then([=]($<Playlist> playlist) {
			return db->cacheLibraryItems({
				MediaProvider::LibraryItem{
					.mediaItem = playlist,
					.addedAt = Date::now()
				}
			}).map(nullptr, [=]() -> $<Playlist> {
				return playlist;
			});
		});
	}



	#pragma mark Saving / Following

	Promise<void> MediaLibrary::saveTrack($<Track> track) {
		auto mediaProvider = track->mediaProvider();
		auto date = Date::now();
		if(mediaProvider->canSaveTracks()) {
			// save track through provider
			auto dbPromise = db->getSavedTrackJson(track->uri());
			auto dataPromise = track->fetchDataIfNeeded();
			return mediaProvider->saveTrack(track->uri())
			.then([=]() {
				return dataPromise;
			}).then([=]() {
				return dbPromise;
			}).then([=](auto json) {
				auto addedAt = json["addedAt"];
				return db->cacheLibraryItems({
					MediaProvider::LibraryItem{
						.mediaItem = track,
						.addedAt = addedAt.is_string() ? Date::maybeFromISOString(addedAt.string_value()).valueOr(date) : date
					}
				});
			});
		} else {
			// save track through soundhole
			auto soundholeProvider = dynamic_cast<SoundHoleMediaProvider*>(mediaProviderStash->getMediaProvider(SoundHoleMediaProvider::NAME));
			if(soundholeProvider == nullptr || !soundholeProvider->isLoggedIn()) {
				return Promise<void>::reject(std::invalid_argument(mediaProvider->name()+" provider cannot save tracks"));
			}
			auto dbPromise = db->getSavedTrackJson(track->uri());
			auto dataPromise = track->fetchDataIfNeeded();
			return soundholeProvider->saveTrack(track->uri(), mediaProvider)
			.then([=]() {
				return dataPromise;
			}).then([=]() {
				return dbPromise;
			}).then([=](auto json) {
				auto addedAt = json["addedAt"];
				return db->cacheLibraryItems({
					MediaProvider::LibraryItem{
						.mediaItem = track,
						.addedAt = addedAt.is_string() ? Date::maybeFromISOString(addedAt.string_value()).valueOr(date) : date
					}
				});
			});
		}
	}

	Promise<void> MediaLibrary::unsaveTrack($<Track> track) {
		auto mediaProvider = track->mediaProvider();
		if(mediaProvider->canSaveTracks()) {
			// unsave track through provider
			return mediaProvider->unsaveTrack(track->uri())
			.then([=]() {
				return db->deleteSavedTrack(track->uri());
			});
		} else {
			// unsave track through soundhole
			auto soundholeProvider = dynamic_cast<SoundHoleMediaProvider*>(mediaProviderStash->getMediaProvider(SoundHoleMediaProvider::NAME));
			if(soundholeProvider == nullptr || !soundholeProvider->isLoggedIn()) {
				return Promise<void>::reject(std::invalid_argument(mediaProvider->name()+" provider cannot save tracks"));
			}
			return soundholeProvider->unsaveTrack(track->uri(), mediaProvider)
			.then([=]() {
				return db->deleteSavedTrack(track->uri());
			});
		}
	}

	Promise<ArrayList<bool>> MediaLibrary::hasSavedTracks(ArrayList<$<Track>> tracks) {
		return db->hasSavedTracks(tracks.map([](auto track){return track->uri();}));
	}



	Promise<void> MediaLibrary::saveAlbum($<Album> album) {
		auto mediaProvider = album->mediaProvider();
		auto date = Date::now();
		if(mediaProvider->canSaveAlbums()) {
			// save album through provider
			auto dbPromise = db->getSavedAlbumJson(album->uri());
			auto dataPromise = album->fetchDataIfNeeded();
			return mediaProvider->saveAlbum(album->uri())
			.then([=]() {
				return dataPromise;
			}).then([=]() {
				return dbPromise;
			}).then([=](auto json) {
				auto addedAt = json["addedAt"];
				return db->cacheLibraryItems({
					MediaProvider::LibraryItem{
						.mediaItem = album,
						.addedAt = addedAt.is_string() ? Date::maybeFromISOString(addedAt.string_value()).valueOr(date) : date
					}
				});
			});
		} else {
			// save album through soundhole
			auto soundholeProvider = dynamic_cast<SoundHoleMediaProvider*>(mediaProviderStash->getMediaProvider(SoundHoleMediaProvider::NAME));
			if(soundholeProvider == nullptr || !soundholeProvider->isLoggedIn()) {
				return Promise<void>::reject(std::invalid_argument(mediaProvider->name()+" provider cannot save albums"));
			}
			auto dbPromise = db->getSavedAlbumJson(album->uri());
			auto dataPromise = album->fetchDataIfNeeded();
			return soundholeProvider->saveAlbum(album->uri(), mediaProvider)
			.then([=]() {
				return dataPromise;
			}).then([=]() {
				return dbPromise;
			}).then([=](auto json) {
				auto addedAt = json["addedAt"];
				return db->cacheLibraryItems({
					MediaProvider::LibraryItem{
						.mediaItem = album,
						.addedAt = addedAt.is_string() ? Date::maybeFromISOString(addedAt.string_value()).valueOr(date) : date
					}
				});
			});
		}
	}

	Promise<void> MediaLibrary::unsaveAlbum($<Album> album) {
		auto mediaProvider = album->mediaProvider();
		if(mediaProvider->canSaveAlbums()) {
			// unsave album through provider
			return mediaProvider->unsaveAlbum(album->uri())
			.then([=]() {
				return db->deleteSavedAlbum(album->uri());
			});
		} else {
			// unsave album through soundhole
			auto soundholeProvider = dynamic_cast<SoundHoleMediaProvider*>(mediaProviderStash->getMediaProvider(SoundHoleMediaProvider::NAME));
			if(soundholeProvider == nullptr || !soundholeProvider->isLoggedIn()) {
				return Promise<void>::reject(std::invalid_argument(mediaProvider->name()+" provider cannot save albums"));
			}
			return soundholeProvider->unsaveAlbum(album->uri(), mediaProvider)
			.then([=]() {
				return db->deleteSavedAlbum(album->uri());
			});
		}
	}

	Promise<ArrayList<bool>> MediaLibrary::hasSavedAlbums(ArrayList<$<Album>> albums) {
		return db->hasSavedAlbums(albums.map([](auto album){return album->uri();}));
	}



	Promise<void> MediaLibrary::savePlaylist($<Playlist> playlist) {
		auto mediaProvider = playlist->mediaProvider();
		auto date = Date::now();
		if(mediaProvider->canSavePlaylists()) {
			// save playlist through provider
			auto dbPromise = db->getSavedPlaylistJson(playlist->uri());
			auto dataPromise = playlist->fetchDataIfNeeded();
			return mediaProvider->savePlaylist(playlist->uri())
			.then([=]() {
				return dataPromise;
			}).then([=]() {
				return dbPromise;
			}).then([=](auto json) {
				auto addedAt = json["addedAt"];
				return db->cacheLibraryItems({
					MediaProvider::LibraryItem{
						.mediaItem = playlist,
						.addedAt = addedAt.is_string() ? Date::maybeFromISOString(addedAt.string_value()).valueOr(date) : date
					}
				});
			});
		}
		else {
			// save playlist through soundhole
			auto soundholeProvider = dynamic_cast<SoundHoleMediaProvider*>(mediaProviderStash->getMediaProvider(SoundHoleMediaProvider::NAME));
			if(soundholeProvider == nullptr || !soundholeProvider->isLoggedIn()) {
				return Promise<void>::reject(std::invalid_argument(mediaProvider->name()+" provider cannot save playlists"));
			}
			auto dbPromise = db->getSavedPlaylistJson(playlist->uri());
			auto dataPromise = playlist->fetchDataIfNeeded();
			return soundholeProvider->savePlaylist(playlist->uri(), mediaProvider)
			.then([=]() {
				return dataPromise;
			}).then([=]() {
				return dbPromise;
			}).then([=](auto json) {
				auto addedAt = json["addedAt"];
				return db->cacheLibraryItems({
					MediaProvider::LibraryItem{
						.mediaItem = playlist,
						.addedAt = addedAt.is_string() ? Date::maybeFromISOString(addedAt.string_value()).valueOr(date) : date
					}
				});
			});
		}
	}

	Promise<void> MediaLibrary::unsavePlaylist($<Playlist> playlist) {
		auto mediaProvider = playlist->mediaProvider();
		if(mediaProvider->canSavePlaylists()) {
			// unsave playlist through provider
			return mediaProvider->unsavePlaylist(playlist->uri())
			.then([=]() {
				return db->deleteSavedPlaylist(playlist->uri());
			});
		}
		else {
			// unsave playlist through soundhole
			auto soundholeProvider = dynamic_cast<SoundHoleMediaProvider*>(mediaProviderStash->getMediaProvider(SoundHoleMediaProvider::NAME));
			if(soundholeProvider == nullptr || !soundholeProvider->isLoggedIn()) {
				return Promise<void>::reject(std::invalid_argument(mediaProvider->name()+" provider cannot save playlists"));
			}
			return soundholeProvider->unsavePlaylist(playlist->uri(), mediaProvider)
			.then([=]() {
				return db->deleteSavedPlaylist(playlist->uri());
			});
		}
	}

	Promise<ArrayList<bool>> MediaLibrary::hasSavedPlaylists(ArrayList<$<Playlist>> playlists) {
		return db->hasSavedPlaylists(playlists.map([](auto playlist){return playlist->uri();}));
	}



	Promise<void> MediaLibrary::followArtist($<Artist> artist) {
		auto mediaProvider = artist->mediaProvider();
		auto date = Date::now();
		if(mediaProvider->canFollowArtists()) {
			// follow artist through provider
			auto dbPromise = db->getFollowedArtistJson(artist->uri());
			auto dataPromise = artist->fetchDataIfNeeded();
			return mediaProvider->followArtist(artist->uri())
			.then([=]() {
				return dataPromise;
			}).then([=]() {
				return dbPromise;
			}).then([=](auto json) {
				auto addedAt = json["addedAt"];
				return db->cacheLibraryItems({
					MediaProvider::LibraryItem{
						.mediaItem = artist,
						.addedAt = addedAt.is_string() ? Date::maybeFromISOString(addedAt.string_value()).valueOr(date) : date
					}
				});
			});
		}
		else {
			// follow artist through soundhole
			auto soundholeProvider = dynamic_cast<SoundHoleMediaProvider*>(mediaProviderStash->getMediaProvider(SoundHoleMediaProvider::NAME));
			if(soundholeProvider == nullptr || !soundholeProvider->isLoggedIn()) {
				return Promise<void>::reject(std::invalid_argument(mediaProvider->name()+" provider cannot follow artists"));
			}
			auto dbPromise = db->getFollowedArtistJson(artist->uri());
			auto dataPromise = artist->fetchDataIfNeeded();
			return soundholeProvider->followArtist(artist->uri(), mediaProvider)
			.then([=]() {
				return dataPromise;
			}).then([=]() {
				return dbPromise;
			}).then([=](auto json) {
				auto addedAt = json["addedAt"];
				return db->cacheLibraryItems({
					MediaProvider::LibraryItem{
						.mediaItem = artist,
						.addedAt = addedAt.is_string() ? Date::maybeFromISOString(addedAt.string_value()).valueOr(date) : date
					}
				});
			});
		}
	}

	Promise<void> MediaLibrary::unfollowArtist($<Artist> artist) {
		auto mediaProvider = artist->mediaProvider();
		if(mediaProvider->canSavePlaylists()) {
			// unfollow artist through provider
			return mediaProvider->unfollowArtist(artist->uri())
			.then([=]() {
				return db->deleteFollowedArtist(artist->uri());
			});
		}
		else {
			// unfollow artist through soundhole
			auto soundholeProvider = dynamic_cast<SoundHoleMediaProvider*>(mediaProviderStash->getMediaProvider(SoundHoleMediaProvider::NAME));
			if(soundholeProvider == nullptr || !soundholeProvider->isLoggedIn()) {
				return Promise<void>::reject(std::invalid_argument(mediaProvider->name()+" provider cannot save playlists"));
			}
			return soundholeProvider->unfollowArtist(artist->uri(), mediaProvider)
			.then([=]() {
				return db->deleteFollowedArtist(artist->uri());
			});
		}
	}

	Promise<ArrayList<bool>> MediaLibrary::hasFollowedArtists(ArrayList<$<Artist>> artists) {
		return db->hasFollowedArtists(artists.map([](auto artist){return artist->uri();}));
	}



	Promise<void> MediaLibrary::followUserAccount($<UserAccount> userAccount) {
		auto mediaProvider = userAccount->mediaProvider();
		if(mediaProvider->handlesUsersAsArtists()) {
			return this->followArtist(mediaProvider->artist(Artist::Data{{
				.partial = true,
				.type = "user",
				.name = userAccount->name(),
				.uri = userAccount->uri(),
				.images = userAccount->images()
				},
				.description = std::nullopt
			}));
		}
		auto date = Date::now();
		if(mediaProvider->canFollowUsers()) {
			// follow user through provider
			auto dbPromise = db->getFollowedUserAccountJson(userAccount->uri());
			auto dataPromise = userAccount->fetchDataIfNeeded();
			return mediaProvider->followUser(userAccount->uri())
			.then([=]() {
				return dataPromise;
			}).then([=]() {
				return dbPromise;
			}).then([=](auto json) {
				auto addedAt = json["addedAt"];
				return db->cacheLibraryItems({
					MediaProvider::LibraryItem{
						.mediaItem = userAccount,
						.addedAt = addedAt.is_string() ? Date::maybeFromISOString(addedAt.string_value()).valueOr(date) : date
					}
				});
			});
		}
		else {
			// follow user through soundhole
			auto soundholeProvider = dynamic_cast<SoundHoleMediaProvider*>(mediaProviderStash->getMediaProvider(SoundHoleMediaProvider::NAME));
			if(soundholeProvider == nullptr || !soundholeProvider->isLoggedIn()) {
				return Promise<void>::reject(std::invalid_argument(mediaProvider->name()+" provider cannot follow artists"));
			}
			auto dbPromise = db->getFollowedUserAccountJson(userAccount->uri());
			auto dataPromise = userAccount->fetchDataIfNeeded();
			return soundholeProvider->followUser(userAccount->uri(), mediaProvider)
			.then([=]() {
				return dataPromise;
			}).then([=]() {
				return dbPromise;
			}).then([=](auto json) {
				auto addedAt = json["addedAt"];
				return db->cacheLibraryItems({
					MediaProvider::LibraryItem{
						.mediaItem = userAccount,
						.addedAt = addedAt.is_string() ? Date::maybeFromISOString(addedAt.string_value()).valueOr(date) : date
					}
				});
			});
		}
	}

	Promise<void> MediaLibrary::unfollowUserAccount($<UserAccount> userAccount) {
		auto mediaProvider = userAccount->mediaProvider();
		if(mediaProvider->handlesUsersAsArtists()) {
			return this->unfollowArtist(mediaProvider->artist(Artist::Data{{
				.partial = true,
				.type = "user",
				.name = userAccount->name(),
				.uri = userAccount->uri(),
				.images = userAccount->images()
				},
				.description = std::nullopt
			}));
		}
		if(mediaProvider->canFollowUsers()) {
			// unfollow user through provider
			return mediaProvider->unfollowUser(userAccount->uri())
			.then([=]() {
				return db->deleteFollowedUserAccount(userAccount->uri());
			});
		}
		else {
			// unfollow user through soundhole
			auto soundholeProvider = dynamic_cast<SoundHoleMediaProvider*>(mediaProviderStash->getMediaProvider(SoundHoleMediaProvider::NAME));
			if(soundholeProvider == nullptr || !soundholeProvider->isLoggedIn()) {
				return Promise<void>::reject(std::invalid_argument(mediaProvider->name()+" provider cannot save playlists"));
			}
			return soundholeProvider->unfollowUser(userAccount->uri(), mediaProvider)
			.then([=]() {
				return db->deleteFollowedUserAccount(userAccount->uri());
			});
		}
	}

	Promise<ArrayList<bool>> MediaLibrary::hasFollowedUserAccounts(ArrayList<$<UserAccount>> userAccounts) {
		auto uris = userAccounts.map([](auto user){return user->uri();});
		LinkedList<String> userURIs;
		LinkedList<String> artistURIs;
		for(auto& userAccount : userAccounts) {
			auto mediaProvider = userAccount->mediaProvider();
			if(mediaProvider->handlesUsersAsArtists()) {
				artistURIs.pushBack(userAccount->uri());
			} else {
				userURIs.pushBack(userAccount->uri());
			}
		}
		return tuplePromiseOf(
			db->hasFollowedUserAccounts(userURIs),
			db->hasFollowedArtists(artistURIs)
		).map([=](auto tuple) -> ArrayList<bool> {
			auto& [ userResults, artistResults ] = tuple;
			ArrayList<bool> followed;
			followed.reserve(uris.size());
			auto userResultsIt = userResults.begin();
			auto userURIsIt = userURIs.begin();
			auto artistResultsIt = artistResults.begin();
			auto artistURIsIt = artistURIs.begin();
			for(auto& uri : uris) {
				if(userURIsIt != userURIs.end() && *userURIsIt == uri) {
					followed.pushBack(*userResultsIt);
					userResultsIt++;
					userURIsIt++;
				}
				else if(artistURIsIt != artistURIs.end() && *artistURIsIt == uri) {
					followed.pushBack(*artistResultsIt);
					artistResultsIt++;
					artistURIsIt++;
				}
				else {
					throw std::logic_error("could not find matching URI for "+uri);
				}
			}
			return followed;
		});
	}
}

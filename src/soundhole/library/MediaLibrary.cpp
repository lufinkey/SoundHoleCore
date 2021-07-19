//
//  MediaLibrary.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 6/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "MediaLibrary.hpp"
#include "MediaLibraryProxyProvider.hpp"
#include <soundhole/utils/Utils.hpp>

namespace sh {
	MediaLibrary::MediaLibrary(Options options)
	: libraryProxyProvider(nullptr), db(nullptr), mediaProviderStash(options.mediaProviderStash) {
		libraryProxyProvider = new MediaLibraryProxyProvider(this);
		db = new MediaDatabase({
		   .path=options.dbPath,
		   .stash=options.mediaProviderStash
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
		return synchronizeQueue.runSingle(runOptions, [=](auto task) {
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
			return generate<void>([=](auto yield) {
				auto syncResumeStr = await(db->getStateValue("syncResumeData_"+libraryProvider->name(), ""));
				Json syncResumeData;
				if(syncResumeStr.length() > 0) {
					std::string syncResumeError;
					syncResumeData = Json::parse(syncResumeStr, syncResumeError);
					if(syncResumeError.length() > 0) {
						syncResumeData = Json();
					}
				}
				yield();
				
				// sync provider library
				auto libraryGenerator = libraryProvider->generateLibrary({.resumeData=syncResumeData});
				while(true) {
					// get library items from provider until success
					task->setStatusText("Synchronizing "+libraryProvider->displayName()+" library");
					MediaProvider::LibraryItemGenerator::YieldResult yieldResult;
					try {
						yieldResult = await(libraryGenerator.next());
					} catch(Error& error) {
						task->setStatusText("Error: "+error.toString());
						yield();
						auto retryAfter = error.getDetail("retryAfter").maybeAs<double>().value_or(0);
						if(retryAfter > 0) {
							std::this_thread::sleep_for(std::chrono::milliseconds((long)(retryAfter * 1000)));
						} else {
							std::this_thread::sleep_for(std::chrono::seconds(2));
						}
						yield();
						continue;
					} catch(std::exception& error) {
						task->setStatusText((String)"Error: "+error.what());
						yield();
						std::this_thread::sleep_for(std::chrono::seconds(2));
						yield();
						continue;
					}
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
								await(libraryItem.mediaItem->fetchDataIfNeeded());
								task->setStatusText("Synchronizing "+libraryProvider->displayName()+" library");
							}
							// increment index
							libraryItemIndex += 1;
							// fetch tracks if item is a collection
							if(auto collection = std::dynamic_pointer_cast<TrackCollection>(libraryItem.mediaItem)) {
								auto existingCollection = maybeTryAwait(db->getTrackCollectionJson(collection->uri()), Json());
								auto existingVersionId = existingCollection["versionId"];
								if(existingVersionId.is_string() && collection->versionId() == existingVersionId.string_value()) {
									continue;
								}
								await(db->cacheTrackCollections({ collection }));
								// cache collection items
								size_t itemsOffset = 0;
								task->setStatusText("Synchronizing "+libraryProvider->displayName()+" "+collection->type()+" "+collection->name());
								auto collectionItemGenerator = collection->generateItems();
								while(true) {
									TrackCollection::ItemGenerator::YieldResult itemsYieldResult;
									try {
										itemsYieldResult = await(collectionItemGenerator.next());
									} catch(Error& error) {
										task->setStatusText("Error: "+error.toString());
										yield();
										auto retryAfter = error.getDetail("retryAfter").maybeAs<double>().value_or(0);
										if(retryAfter > 0) {
											std::this_thread::sleep_for(std::chrono::milliseconds((long)(retryAfter * 1000)));
										} else {
											std::this_thread::sleep_for(std::chrono::seconds(2));
										}
										yield();
										continue;
									} catch(std::exception& error) {
										task->setStatusText((String)"Error: "+error.what());
										yield();
										std::this_thread::sleep_for(std::chrono::seconds(2));
										yield();
										continue;
									}
									if(itemsYieldResult.value) {
										size_t nextOffset = itemsOffset + itemsYieldResult.value->size();
										await(db->cacheTrackCollectionItems(collection, sql::IndexRange{
											.startIndex = itemsOffset,
											.endIndex = nextOffset
										}));
										itemsOffset += itemsYieldResult.value->size();
										if(collection->itemCount()) {
											task->setStatusProgress(itemProgressStart + (((double)nextOffset / (double)collection->itemCount().value()) * itemProgressDiff));
										}
										// TODO reset collection items
									}
									if(itemsYieldResult.done) {
										break;
									}
									yield();
									std::this_thread::sleep_for(std::chrono::milliseconds(200));
									yield();
								}
								// update versionId
								await(db->updateTrackCollectionVersionId(collection));
								// sync library item
								await(db->cacheLibraryItems({ libraryItem }));
							} else {
								task->setStatusProgress(itemProgressStart + itemProgressDiff);
							}
							task->setStatusText("Synchronizing "+libraryProvider->displayName()+" library");
							yield();
						}
						
						// sync library items to database
						auto cacheOptions = MediaDatabase::CacheOptions{
							.dbState = {
								{ ("syncResumeData_"+libraryProvider->name()), yieldResult.value->resumeData.dump() }
							}
						};
						await(db->cacheLibraryItems(yieldResult.value->items, cacheOptions));
						task->setStatusProgress(yieldResult.value->progress);
					}
					if(yieldResult.done) {
						break;
					}
					yield();
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
					yield();
				}
				
				task->setStatus({
					.progress = 1.0,
					.text = "Finished synchronizing "+libraryProvider->displayName()+" library"
				});
			});
		});
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
				}).except([=](std::exception_ptr error) {
					task->removeCancelListener(cancelListenerId);
					syncTask->removeStatusChangeListener(statusChangeListenerId);
					task->setStatus({
						.progress = ((double)providerIndex + 1.0) / (double)providerCount,
						.text = (String)"Error syncing "+provider->displayName()+" library: "+utils::getExceptionDetails(error).message
					});
					return async<void>([=]() {
						for(size_t i=0; i<6; i++) {
							if(task->isCancelled()) {
								return;
							}
							std::this_thread::sleep_for(std::chrono::milliseconds(500));
						}
					});
				}).map([=]() -> YieldResult {
					return YieldResult{
						.done = false
					};
				});
			});
		});
	}




	#pragma mark Library Tracks

	Promise<$<MediaLibraryTracksCollection>> MediaLibrary::getLibraryTracksCollection(GetLibraryTracksOptions options) {
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



	Promise<$<Playlist>> MediaLibrary::createPlaylist(String name, MediaProvider* provider, CreatePlaylistOptions options) {
		return provider->createPlaylist(name, options).then([=]($<Playlist> playlist) {
			return db->cacheLibraryItems({
				MediaProvider::LibraryItem{
					.libraryProvider=provider,
					.mediaItem=playlist,
					.addedAt=String() // TODO add a default date possibly
				}
			}).map([=]() -> $<Playlist> {
				return playlist;
			});
		});
	}



	Promise<void> MediaLibrary::saveTrack($<Track> track) {
		auto mediaProvider = track->mediaProvider();
		auto dbPromise = db->getSavedTrackJson(track->uri(), mediaProvider->name());
		return mediaProvider->saveTrack(track->uri())
		.then([=]() {
			auto date = DateTime();
			auto _dbPromise = dbPromise;
			return _dbPromise.then([=](auto json) {
				return db->cacheLibraryItems({
					MediaProvider::LibraryItem{
						.libraryProvider = mediaProvider,
						.mediaItem = track,
						.addedAt = json.is_null() ? date.toISO8601String() : String(json["addedAt"].string_value())
					}
				});
			});
		});
	}

	Promise<void> MediaLibrary::unsaveTrack($<Track> track) {
		auto mediaProvider = track->mediaProvider();
		return mediaProvider->unsaveTrack(track->uri())
		.then([=]() {
			return db->deleteSavedTrack(track->uri(), mediaProvider->name());
		});
	}

	Promise<ArrayList<bool>> MediaLibrary::hasSavedTracks(ArrayList<$<Track>> tracks) {
		return db->hasSavedTracks(tracks.map([](auto track){return track->uri();}));
	}



	Promise<void> MediaLibrary::saveAlbum($<Album> album) {
		auto mediaProvider = album->mediaProvider();
		auto dbPromise = db->getSavedAlbumJson(album->uri(), mediaProvider->name());
		return mediaProvider->saveAlbum(album->uri())
		.then([=]() {
			auto date = DateTime();
			auto _dbPromise = dbPromise;
			return _dbPromise.then([=](auto json) {
				return db->cacheLibraryItems({
					MediaProvider::LibraryItem{
						.libraryProvider = mediaProvider,
						.mediaItem = album,
						.addedAt = json.is_null() ? date.toISO8601String() : String(json["addedAt"].string_value())
					}
				});
			});
		});
	}

	Promise<void> MediaLibrary::unsaveAlbum($<Album> album) {
		auto mediaProvider = album->mediaProvider();
		return mediaProvider->unsaveAlbum(album->uri())
		.then([=]() {
			return db->deleteSavedAlbum(album->uri(), mediaProvider->name());
		});
	}

	Promise<ArrayList<bool>> MediaLibrary::hasSavedAlbums(ArrayList<$<Album>> albums) {
		return db->hasSavedAlbums(albums.map([](auto album){return album->uri();}));
	}
}

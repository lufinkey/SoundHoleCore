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
	: libraryProxyProvider(nullptr), db(nullptr) {
		libraryProxyProvider = new MediaLibraryProxyProvider(options.mediaProviders);
		db = new MediaDatabase({
		   .path=options.dbPath,
		   .stash=libraryProxyProvider
		});
	}

	MediaLibrary::~MediaLibrary() {
		delete db;
		delete libraryProxyProvider;
	}

	Promise<void> MediaLibrary::initialize() {
		return Promise<void>::resolve().then([=]() {
			db->open();
			return db->initialize();
		});
	}

	Promise<void> MediaLibrary::resetDatabase() {
		return db->reset();
	}

	MediaDatabase* MediaLibrary::database() {
		return db;
	}

	const MediaDatabase* MediaLibrary::database() const {
		return db;
	}
	
	MediaProvider* MediaLibrary::getMediaProvider(String name) {
		return libraryProxyProvider->getMediaProvider(name);
	}

	ArrayList<MediaProvider*> MediaLibrary::getMediaProviders() {
		return libraryProxyProvider->getMediaProviders();
	}
	
	bool MediaLibrary::isSynchronizingLibrary(String libraryProviderName) {
		auto taskNode = getSynchronizeLibraryTask(libraryProviderName);
		if(taskNode) {
			return true;
		}
		return false;
	}

	bool MediaLibrary::isSynchronizingLibraries() {
		for(MediaProvider* provider : libraryProxyProvider->getMediaProviders()) {
			if(isSynchronizingLibrary(provider->name())) {
				return true;
			}
		}
		return false;
	}

	Optional<AsyncQueue::TaskNode> MediaLibrary::getSynchronizeLibraryTask(String libraryProviderName) {
		return synchronizeQueue.getTaskWithTag("sync:"+libraryProviderName);
	}

	Optional<AsyncQueue::TaskNode> MediaLibrary::getSynchronizeAllLibrariesTask() {
		return synchronizeAllQueue.getTaskWithTag("sync:all");
	}

	AsyncQueue::TaskNode MediaLibrary::synchronizeProviderLibrary(String libraryProviderName) {
		auto provider = getMediaProvider(libraryProviderName);
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
						task->setStatusText("Error: "+error.getMessage());
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
										task->setStatusText("Error: "+error.getMessage());
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
			for(auto provider : libraryProxyProvider->getMediaProviders()) {
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
			providers->insert(providers->begin(), queuedProviders.template map<MediaProvider*>([](auto& node) {
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
				}).template map<YieldResult>([=]() {
					return YieldResult{
						.done = false
					};
				});
			});
		});
	}

	String MediaLibrary::getLibraryTracksCollectionURI(GetLibraryTracksFilters filters) const {
		String uri = "medialibrary:collection:libraryTracks";
		LinkedList<String> query;
		if(filters.libraryProvider != nullptr) {
			query.pushBack("libraryProvider="+filters.libraryProvider->name());
		}
		query.pushBack("orderBy="+sql::LibraryItemOrderBy_toString(filters.orderBy));
		query.pushBack("order="+sql::Order_toString(filters.order));
		return String::join({ uri,"?",String::join(query) });
	}

	Promise<$<MediaLibraryTracksCollection>> MediaLibrary::getLibraryTracksCollection(GetLibraryTracksFilters filters, GetLibraryTracksOptions options) {
		size_t startIndex = options.offset.valueOr(0);
		return db->getSavedTracksJson({
			.startIndex=startIndex,
			.endIndex=(startIndex + options.limit.valueOr(24))
		}, {
			.libraryProvider=(filters.libraryProvider != nullptr) ? filters.libraryProvider->name() : String(),
			.orderBy=filters.orderBy,
			.order=filters.order
		}).map<$<MediaLibraryTracksCollection>>(nullptr, [=](MediaDatabase::GetJsonItemsListResult results) {
			auto items = std::map<size_t,MediaLibraryTracksCollectionItem::Data>();
			size_t itemIndex = startIndex;
			for(auto& jsonItem : results.items) {
				auto jsonItemObj = jsonItem.object_items();
				auto trackJsonIt = jsonItemObj.find("mediaItem");
				if(trackJsonIt != jsonItemObj.end()) {
					auto trackJson = trackJsonIt->second;
					jsonItemObj.erase(trackJsonIt);
					jsonItemObj["track"] = trackJson;
				}
				items.insert_or_assign(itemIndex, MediaLibraryTracksCollectionItem::Data::fromJson(jsonItemObj, this->libraryProxyProvider));
				itemIndex++;
			}
			return MediaLibraryTracksCollection::new$(db, this->libraryProxyProvider, MediaLibraryTracksCollection::Data{{{
				.partial = false,
				.type = "libraryTracks",
				.name = filters.libraryProvider ? ("My "+filters.libraryProvider->displayName()+" Tracks") : "My Tracks",
				.uri = getLibraryTracksCollectionURI(filters),
				.images = ArrayList<MediaItem::Image>{}
				},
				.versionId = String(),
				.itemCount = results.total,
				.items = items,
				},
				.filters = MediaLibraryTracksCollection::Filters{
					.libraryProvider = filters.libraryProvider,
					.orderBy = filters.orderBy,
					.order = filters.order
				}
			});
		});
	}

	Promise<LinkedList<$<Album>>> MediaLibrary::getLibraryAlbums(GetLibraryAlbumsFilters filters) {
		return db->getSavedAlbumsJson({
			.libraryProvider = (filters.libraryProvider != nullptr) ? filters.libraryProvider->name() : String(),
			.orderBy=filters.orderBy,
			.order=filters.order
		}).map<LinkedList<$<Album>>>(nullptr, [=](MediaDatabase::GetJsonItemsListResult results) {
			return results.items.map<$<Album>>([=](Json json) {
				auto mediaItemJson = json["mediaItem"];
				auto providerName = mediaItemJson["provider"];
				if(!providerName.is_string()) {
					throw std::invalid_argument("album provider must be a string");
				}
				auto provider = this->libraryProxyProvider->getMediaProvider(providerName.string_value());
				if(provider == nullptr) {
					throw std::invalid_argument("invalid provider name for album: "+providerName.string_value());
				}
				return provider->album(Album::Data::fromJson(mediaItemJson, this->libraryProxyProvider));
			});
		});
	}

	MediaLibrary::LibraryAlbumsGenerator MediaLibrary::generateLibraryAlbums(GetLibraryAlbumsFilters filters, GenerateLibraryAlbumsOptions options) {
		auto offset = fgl::new$<size_t>(options.offset);
		using YieldResult = typename LibraryAlbumsGenerator::YieldResult;
		return LibraryAlbumsGenerator([=]() {
			size_t startIndex = *offset;
			size_t endIndex = (size_t)-1;
			if(options.chunkSize != (size_t)-1) {
				endIndex = startIndex + options.chunkSize;
			}
			return db->getSavedAlbumsJson({
				.libraryProvider = (filters.libraryProvider != nullptr) ? filters.libraryProvider->name() : String(),
				.range = sql::IndexRange{
					.startIndex = startIndex,
					.endIndex = endIndex
				},
				.orderBy=filters.orderBy,
				.order=filters.order
			}).map<YieldResult>(nullptr, [=](MediaDatabase::GetJsonItemsListResult results) {
				auto albums = results.items.map<$<Album>>([=](Json json) {
					auto mediaItemJson = json["mediaItem"];
					auto providerName = mediaItemJson["provider"];
					if(!providerName.is_string()) {
						throw std::invalid_argument("album provider must be a string");
					}
					auto provider = this->libraryProxyProvider->getMediaProvider(providerName.string_value());
					if(provider == nullptr) {
						throw std::invalid_argument("invalid provider name for album: "+providerName.string_value());
					}
					return provider->album(Album::Data::fromJson(mediaItemJson, this->libraryProxyProvider));
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

	Promise<LinkedList<$<Playlist>>> MediaLibrary::getLibraryPlaylists(GetLibraryPlaylistsFilters filters) {
		return db->getSavedPlaylistsJson({
			.libraryProvider = (filters.libraryProvider != nullptr) ? filters.libraryProvider->name() : String(),
			.orderBy=filters.orderBy,
			.order=filters.order
		}).map<LinkedList<$<Playlist>>>(nullptr, [=](MediaDatabase::GetJsonItemsListResult results) {
			return results.items.map<$<Playlist>>([=](Json json) {
				auto mediaItemJson = json["mediaItem"];
				auto providerName = mediaItemJson["provider"];
				if(!providerName.is_string()) {
					throw std::invalid_argument("playlist provider must be a string");
				}
				auto provider = this->libraryProxyProvider->getMediaProvider(providerName.string_value());
				if(provider == nullptr) {
					throw std::invalid_argument("invalid provider name for playlist: "+providerName.string_value());
				}
				return provider->playlist(Playlist::Data::fromJson(mediaItemJson, this->libraryProxyProvider));
			});
		});
	}

	MediaLibrary::LibraryPlaylistsGenerator MediaLibrary::generateLibraryPlaylists(GetLibraryPlaylistsFilters filters, GenerateLibraryPlaylistsOptions options) {
		auto offset = fgl::new$<size_t>(options.offset);
		using YieldResult = typename LibraryPlaylistsGenerator::YieldResult;
		return LibraryPlaylistsGenerator([=]() {
			size_t startIndex = *offset;
			size_t endIndex = (size_t)-1;
			if(options.chunkSize != (size_t)-1) {
				endIndex = startIndex + options.chunkSize;
			}
			return db->getSavedPlaylistsJson({
				.libraryProvider = (filters.libraryProvider != nullptr) ? filters.libraryProvider->name() : String(),
				.range = sql::IndexRange{
					.startIndex = startIndex,
					.endIndex = endIndex
				},
				.orderBy=filters.orderBy,
				.order=filters.order
			}).map<YieldResult>(nullptr, [=](MediaDatabase::GetJsonItemsListResult results) {
				auto playlists = results.items.map<$<Playlist>>([=](Json json) {
					auto mediaItemJson = json["mediaItem"];
					auto providerName = mediaItemJson["provider"];
					if(!providerName.is_string()) {
						throw std::invalid_argument("playlist provider must be a string");
					}
					auto provider = this->libraryProxyProvider->getMediaProvider(providerName.string_value());
					if(provider == nullptr) {
						throw std::invalid_argument("invalid provider name for playlist: "+providerName.string_value());
					}
					return provider->playlist(Playlist::Data::fromJson(mediaItemJson, this->libraryProxyProvider));
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
			}).map<$<Playlist>>([=]() {
				return playlist;
			});
		});
	}
}

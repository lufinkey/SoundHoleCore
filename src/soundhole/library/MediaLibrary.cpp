//
//  MediaLibrary.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 6/2/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "MediaLibrary.hpp"
#include "MediaLibraryProvider.hpp"
#include <soundhole/utils/Utils.hpp>

namespace sh {
	MediaLibrary::MediaLibrary(Options options)
	: libraryProvider(nullptr),
		db(nullptr) {
		libraryProvider = new MediaLibraryProvider(options.mediaProviders);
		db = new MediaDatabase({
		   .path=options.dbPath,
		   .stash=libraryProvider
		});
	}

	MediaLibrary::~MediaLibrary() {
		delete db;
		delete libraryProvider;
	}
	
	MediaProvider* MediaLibrary::getMediaProvider(String name) {
		return libraryProvider->getMediaProvider(name);
	}
	
	bool MediaLibrary::isSynchronizingLibrary(String libraryProviderName) {
		auto taskNode = synchronizeQueue.getTaskWithTag("sync:"+libraryProviderName);
		if(taskNode && !taskNode->task->isDone() && !taskNode->task->isCancelled()) {
			return true;
		}
		return false;
	}

	bool MediaLibrary::isSynchronizingLibraries() {
		for(MediaProvider* provider : libraryProvider->getMediaProviders()) {
			if(isSynchronizingLibrary(provider->name())) {
				return true;
			}
		}
		return false;
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
			.tag=("sync:"+libraryProvider->name())
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
				auto generator = libraryProvider->generateLibrary({.resumeData=syncResumeData});
				while(true) {
					// get tracks from provider until success
					task->setStatusText("Synchronizing "+libraryProvider->displayName()+" library");
					MediaProvider::LibraryItemGenerator::YieldResult yieldResult;
					try {
						yieldResult = await(generator.next());
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
						// sync items to database
						task->setStatusProgress(yieldResult.value->progress);
						auto cacheOptions = MediaDatabase::CacheOptions{
							.dbState = {
								{ ("syncResumeData_"+libraryProvider->name()), yieldResult.value->resumeData.dump() }
							}
						};
						await(db->cacheLibraryItems(yieldResult.value->items, cacheOptions));
					}
					if(yieldResult.done) {
						break;
					}
					yield();
					std::this_thread::sleep_for(std::chrono::milliseconds(200));
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
			.tag="sync:all"
		};
		return synchronizeAllQueue.runSingle(runOptions, [=](auto task) {
			auto successCount = fgl::new$<size_t>(0);
			auto providers = fgl::new$<LinkedList<MediaProvider*>>();
			for(auto provider : libraryProvider->getMediaProviders()) {
				if(provider->hasLibrary()) {
					providers->pushBack(provider);
				}
			}
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
				}
				size_t cancelListenerId = task->addCancelListener([=](auto task) {
					syncTask->cancel();
				});
				size_t statusChangeListenerId = syncTask->addStatusChangeListener([=](auto task, size_t listenerId) {
					task->setStatus({
						.progress = ((double)providerIndex + syncTaskStatus.progress) / (double)providerCount,
						.text = task->getStatus().text
					});
				});
				return taskNode.promise.then([=]() {
					task->removeCancelListener(cancelListenerId);
					syncTask->removeStatusChangeListener(statusChangeListenerId);
					*successCount += 1;
				}).except([=](std::exception_ptr error) {
					task->removeCancelListener(cancelListenerId);
					syncTask->removeStatusChangeListener(statusChangeListenerId);
					task->setStatusText((String)"Error syncing "+provider->displayName()+" library: "+utils::getExceptionDetails(error).message);
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
}
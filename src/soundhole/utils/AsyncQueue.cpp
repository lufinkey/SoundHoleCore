//
//  AsyncQueue.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 11/17/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "AsyncQueue.hpp"

namespace sh {
	AsyncQueue::AsyncQueue(Options options)
	: options(options) {
		//
	}

	size_t AsyncQueue::taskCount() const {
		return taskQueue.size();
	}

	$<AsyncQueue::Task> AsyncQueue::getTaskWithName(const String& name) {
		for(auto& taskNode : taskQueue) {
			if(taskNode.task->getName() == name) {
				return taskNode.task;
			}
		}
		return nullptr;
	}

	$<AsyncQueue::Task> AsyncQueue::getTaskWithTag(const String& tag) {
		for(auto& taskNode : taskQueue) {
			if(taskNode.task->getTag() == tag) {
				return taskNode.task;
			}
		}
		return nullptr;
	}

	LinkedList<$<AsyncQueue::Task>> AsyncQueue::getTasksWithTag(const String& tag) {
		std::unique_lock<std::recursive_mutex> lock(mutex);
		LinkedList<$<AsyncQueue::Task>> tasks;
		for(auto& taskNode : taskQueue) {
			if(taskNode.task->getTag() == tag) {
				tasks.pushBack(taskNode.task);
			}
		}
		return tasks;
	}

	void AsyncQueue::cancelAllTasks() {
		std::unique_lock<std::recursive_mutex> lock(mutex);
		auto tasks = taskQueue;
		for(auto& taskNode : tasks) {
			taskNode.task->cancel();
		}
	}

	void AsyncQueue::cancelTasksWithTag(const String& tag) {
		std::unique_lock<std::recursive_mutex> lock(mutex);
		auto tasks = taskQueue;
		for(auto& taskNode : tasks) {
			if(taskNode.task->getTag() == tag) {
				taskNode.task->cancel();
			}
		}
	}

	void AsyncQueue::cancelTasksWithTags(const ArrayList<String>& tags) {
		std::unique_lock<std::recursive_mutex> lock(mutex);
		auto tasks = taskQueue;
		for(auto& taskNode : tasks) {
			if(tags.contains(taskNode.task->getTag())) {
				taskNode.task->cancel();
			}
		}
	}

	Promise<void> AsyncQueue::waitForCurrentTasks() const {
		return taskQueuePromise.value_or(Promise<void>::resolve());
	}

	Promise<void> AsyncQueue::waitForTasksWithTag(const String& tag) const {
		return Promise<void>::all(ArrayList<Promise<void>>(taskQueue.where([&](auto& taskNode) {
			return (taskNode.task->getTag() == tag);
		}).map<Promise<void>>([&](auto& taskNode) {
			return taskNode.promise;
		})));
	}

	void AsyncQueue::removeTask($<Task> task) {
		std::unique_lock<std::recursive_mutex> lock(mutex);
		taskQueue.removeFirstWhere([&](auto& taskNode) {
			return (taskNode.task == task);
		});
		if(taskQueue.size() == 0) {
			taskQueuePromise = std::nullopt;
		}
	}
}

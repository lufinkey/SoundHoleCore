
import EventEmitter from 'events';
import { awaitDelay, yieldDelay } from './Utils';

type Options = {
	cancelUnfinishedTasks?: boolean
}

type AsyncQueueTaskOptions = {
	tag?: string,
	name?: string,
	metadata?: {[key: string]: any},
	initialProgress?: number,
	initialStatus?: string
}

export type TaskPromise = Promise<any> & {
	readonly cancel: (() => void),
	readonly progress: number | null,
	readonly status: string | null,
	readonly events: EventEmitter
}

export class AsyncQueueTask {
	readonly tag: string | null;
	readonly name: string | null;
	readonly promise: TaskPromise;
	readonly events: EventEmitter;
	private _progress: number | null = null;
	private _status: string | null = null;
	private _cancelled: boolean = false;
	private _done: boolean = false;
	private _error: Error | null = null;

	constructor(options: AsyncQueueTaskOptions, promise: Promise<void>) {
		this.tag = options.tag ?? null;
		this.name = options.name ?? null;
		this.events = new EventEmitter();
		this._progress = options.initialProgress ?? null;
		this._status = options.initialStatus ?? null;
		this.promise = Object.defineProperties(promise, {
			cancel: {
				writable: false,
				value: () => {this.cancel()}
			},
			progress: {
				get: () => (this.progress)
			},
			status: {
				get: () => (this.status)
			},
			done: {
				get: () => (this.done)
			},
			events: {
				writable: false,
				value: this.events
			}
		}) as TaskPromise;
		this.promise.then(() => {
			this._done = true;
		}, (error) => {
			this._error = error;
			this._done = true;
		});
	}

	get progress(): number | null {
		return this._progress;
	}

	set progress(value: number | null) {
		this._progress = value;
		this.events.emit('progress');
	}

	get status(): string | null {
		return this._status;
	}

	set status(value: string | null) {
		this._status = value;
		this.events.emit('statusChange');
	}

	// tells this task to cancel the given task when this task is cancelled.
	// Waits for the given task to finish and returns.
	async join(task: TaskPromise): Promise<void> {
		const onCancel = () => {
			task.cancel();
		};
		let taskError: Error | null = null;
		this.events.addListener('cancel', onCancel);
		try {
			await task;
		}
		catch(error: any) {
			taskError = error as Error;
		}
		this.events.removeListener('cancel', onCancel);
		if(taskError != null) {
			throw taskError;
		}
	}

	cancel() {
		if(!this._cancelled) {
			this._cancelled = true;
			this.events.emit('cancel');
		}
	}

	get cancelled() {
		return this._cancelled;
	}

	get done(): boolean {
		return this._done;
	}
}


export type AsyncQueueFunction = (task: AsyncQueueTask) => (AsyncGenerator<any,any,void> | Promise<any> | void);

type RunOptions = {
	tag?: string;
	cancelMatchingTags?: boolean;
	cancelTags?: Array<string>;
	cancelAll?: boolean;
}

export default class AsyncQueue {
	static Task = AsyncQueueTask;

	private _options: Options;

	private _taskQueue: Array<AsyncQueueTask> = [];
	private _taskQueuePromise: Promise<any> | null = null;
	private _taskCounter = 0;

	constructor(options: Options = {}) {
		this._options = options;
	}

	_createTask(options: AsyncQueueTaskOptions, promise: Promise<void>): AsyncQueueTask {
		const task = new AsyncQueueTask(options, promise);
		this._taskCounter += 1;
		return task;
	}

	_removeTask(task: AsyncQueueTask) {
		// remove task from queue
		const taskIndex = this._taskQueue.indexOf(task);
		if(taskIndex !== -1) {
			this._taskQueue.splice(taskIndex, 1);
		}
		// end task promise if there are no more tasks in queue
		if(this._taskQueue.length === 0) {
			this._taskQueuePromise = null;
		}
	}

	get taskCount(): number {
		return this._taskQueue.length;
	}

	getTaskWithTag(tag: string): AsyncQueueTask | null {
		for(const task of this._taskQueue) {
			if(task.tag === tag) {
				return task;
			}
		}
		return null;
	}

	getTasksWithTag(tag: string): Array<AsyncQueueTask> {
		const tasks = [];
		for(const task of this._taskQueue) {
			if(task.tag === tag) {
				tasks.push(task);
			}
		}
		return tasks;
	}

	async runUntilNextTask(func: () => AsyncGenerator<void,void,void>) {
		const taskCounter = this._taskCounter;
		const generator = func();
		while(true) {
			const { done } = await generator.next();
			if(done) {
				return;
			}
			else if(taskCounter !== this._taskCounter) {
				return;
			}
		}
	}

	async * generateUntilNextTask(func: () => AsyncGenerator<void,void,void>): AsyncGenerator<void,void,void> {
		const taskCounter = this._taskCounter;
		const generator = func();
		while(true) {
			const { done } = await generator.next();
			if(done) {
				return;
			}
			else if(taskCounter !== this._taskCounter) {
				return;
			}
			yield;
		}
	}

	run(options: RunOptions, func: AsyncQueueFunction): TaskPromise;
	run(func: AsyncQueueFunction): TaskPromise;
	
	run(optionsOrFunc: RunOptions | AsyncQueueFunction, funcOrNone?: AsyncQueueFunction): TaskPromise {
		const options: RunOptions = ((typeof optionsOrFunc === 'object') ? optionsOrFunc : null) ?? {};
		const func: AsyncQueueFunction = ((typeof funcOrNone === 'function') ? optionsOrFunc : funcOrNone) as AsyncQueueFunction;
		if(typeof func !== 'function') {
			throw new Error("Invalid queue function");
		}
		if(this._options.cancelUnfinishedTasks || options.cancelAll) {
			this.cancelAllTasks();
		}
		if(options.tag != null && options.cancelMatchingTags) {
			this.cancelTasksWithTag(options.tag);
		}
		if(options.cancelTags) {
			this.cancelTasksWithTags(options.cancelTags);
		}
		let resolveTask: ((value: any) => void);
		let rejectTask: ((error: Error) => void);
		const task = this._createTask({tag: options.tag}, new Promise<any>((resolve, reject) => {
			resolveTask = resolve;
			rejectTask = reject;
		}));
		this._taskQueue.push(task);
		this._taskQueuePromise = (this._taskQueuePromise || Promise.resolve()).then(() => ((async () => {
			await awaitDelay(0);
			// stop if task is cancelled
			if(task.cancelled) {
				this._removeTask(task);
				resolveTask(undefined);
				// emit events
				task.events.emit('resolve');
				task.events.emit('finish');
				return;
			}
			// run task
			let taskError: Error | null = null;
			let taskResult = undefined;
			try {
				const retVal = func(task);
				if(retVal) {
					if(retVal instanceof Promise) {
						taskResult = await retVal;
					}
					else if((typeof retVal === 'object') && typeof retVal.next === 'function') {
						let done: boolean = false;
						while(!task.cancelled && !done) {
							const retYield = await retVal.next();
							done = retYield.done ?? false;
							if(!done) {
								task.events.emit('yield', retYield);
							}
							else {
								taskResult = retYield.value;
							}
						}
					}
					else {
						taskResult = retVal;
					}
				}
				else {
					taskResult = retVal;
				}
			}
			catch(error: any) {
				taskError = error as Error;
			}
			// remove task from queue
			this._removeTask(task);
			// resolve / reject
			if(taskError) {
				rejectTask(taskError);
				task.events.emit('reject');
			}
			else {
				resolveTask(taskResult);
				task.events.emit('resolve');
			}
			// emit event
			task.events.emit('finish');
		})()));
		return task.promise;
	}

	runOne(options: {tag: string} & RunOptions, func: AsyncQueueFunction): TaskPromise {
		const task = this.getTaskWithTag(options.tag);
		if(task != null) {
			return task.promise;
		}
		return this.run({tag: options.tag}, func);
	}

	cancelTasksWithTag(tag: string) {
		for(const task of this._taskQueue) {
			if(task.tag === tag) {
				task.cancel();
			}
		}
	}

	cancelTasksWithTags(tags: Array<string | null>) {
		for(const task of this._taskQueue) {
			if(tags.includes(task.tag)) {
				task.cancel();
			}
		}
	}

	cancelAllTasks() {
		for(const task of this._taskQueue) {
			task.cancel();
		}
	}

	async waitForCurrentTasks() {
		await this._taskQueuePromise;
	}

	async waitForTasksWithTag(tag: string) {
		await Promise.all(this._taskQueue.filter((task: AsyncQueueTask) => (
			(task.tag === tag)
		)).map((task: AsyncQueueTask) => (
			(task.promise)
		)));
	}
}

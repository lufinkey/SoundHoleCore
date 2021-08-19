
export type ContinuousGeneratorResult<Yield> = {
	result?: Yield,
	error?: Error
}

export type ContinuousAsyncGenerator<Yield, Next = void> =
	AsyncGenerator<ContinuousGeneratorResult<Yield>,ContinuousGeneratorResult<Yield>, Next>;

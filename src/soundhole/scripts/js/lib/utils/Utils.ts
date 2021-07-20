
export const json_encode = (data: any): string => {
	return JSON.stringify(data);
}

export const json_decode = (data: string): any => {
	return JSON.parse(data);
}

export const awaitDelay = (timeInterval: number) => {
	return new Promise<void>((resolve, reject) => {
		setTimeout(resolve, timeInterval);
	});
}

export async function* yieldDelay(timeInterval: number, smallerInterval: number = 100) {
	const count = Math.floor(timeInterval / smallerInterval);
	const remainder = (timeInterval % smallerInterval);
	for(let i=0; i<count; i++) {
		await awaitDelay(smallerInterval);
		yield;
	}
	if(remainder > 0) {
		await awaitDelay(remainder);
	}
}

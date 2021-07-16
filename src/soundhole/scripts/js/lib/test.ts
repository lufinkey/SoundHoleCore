
export function testJSFunction(args: any): any {
	console.log("I got this for you");
	console.log(JSON.stringify(args));
	return {
		"ayy": "lmao"
	};
}

export async function testJSAsyncFunction() {
	await Promise.resolve(true);
}


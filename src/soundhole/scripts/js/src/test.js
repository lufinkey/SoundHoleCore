
function testJSFunction(args) {
	console.log("I got this for you");
	console.log(JSON.stringify(args));
	return {
		"ayy": "lmao"
	};
}

async function testJSAsyncFunction() {
	await Promise.resolve(true);
}

module.exports = {
	testJSFunction,
	testJSAsyncFunction
}

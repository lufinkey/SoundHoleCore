
console.log("running test.js");

function testJSFunction(args) {
	console.log("I got this for you");
	console.log(JSON.stringify(args));
	return {
		"ayy": "lmao"
	};
}

console.log("exporting test.js");
module.exports = {
	testJSFunction
}

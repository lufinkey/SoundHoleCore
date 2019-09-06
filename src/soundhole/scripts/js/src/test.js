
const util = require('util');

function testJSFunction(args) {
	console.log("I got this for you");
	console.log(JSON.stringify(args));
	return {
		"ayy": "lmao"
	};
}

module.exports = {
	myFunction
}

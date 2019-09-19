
const YoutubeDL = require('./src/YoutubeDL');
const Test = require('./src/test');

console.log("exporting result");
module.exports = {
	...YoutubeDL,
	...Test
};

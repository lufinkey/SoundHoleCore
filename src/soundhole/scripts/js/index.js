
const HttpClient = require('./src/HttpClient');
const Bandcamp = require('./src/Bandcamp');
const YoutubeDL = require('./src/YoutubeDL');
const Test = require('./src/test');
const Utils = require('./src/Utils');

module.exports = {
	...HttpClient,
	...Bandcamp,
	...YoutubeDL,
	...Test,
	...Utils
};

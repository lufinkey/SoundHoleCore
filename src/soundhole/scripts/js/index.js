
const HttpClient = require('./src/HttpClient');
const Test = require('./src/test');
const Utils = require('./src/Utils');

const Bandcamp = require('bandcamp-api');
const ytdl = require('ytdl-core');
const GoogleDriveStorageProvider = require('./src/storageProviders/GoogleDriveStorageProvider');

module.exports = {
	...HttpClient,
	...Test,
	...Utils,
	Bandcamp,
	GoogleDriveStorageProvider,
	ytdl
};

console.log("module.exports = ", module.exports);

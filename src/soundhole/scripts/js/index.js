
const HttpClient = require('./src/HttpClient');
const Bandcamp = require('./src/Bandcamp');
const YoutubeDL = require('./src/YoutubeDL');
const Test = require('./src/test');

module.exports = Object.assign(module.exports, HttpClient);
module.exports = Object.assign(module.exports, Bandcamp);
module.exports = Object.assign(module.exports, YoutubeDL);
module.exports = Object.assign(module.exports, Test);

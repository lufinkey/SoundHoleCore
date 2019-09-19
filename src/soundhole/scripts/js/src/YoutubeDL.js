
const ytdl = require('ytdl-core');

module.exports = {
	YoutubeDL_download: ytdl,
	YoutubeDL_getBasicInfo: ytdl.getBasicInfo.bind(ytdl),
	YoutubeDL_getInfo: ytdl.getInfo.bind(ytdl),
	YoutubeDL_downloadFromInfo: ytdl.downloadFromInfo.bind(ytdl),
	YoutubeDL_chooseFormat: ytdl.chooseFormat.bind(ytdl),
	YoutubeDL_filterFormats: ytdl.filterFormats.bind(ytdl),
	YoutubeDL_validateId: ytdl.validateID.bind(ytdl),
	YoutubeDL_validateURL: ytdl.validateURL.bind(ytdl),
	YoutubeDL_getURLVideoID: ytdl.getURLVideoID.bind(ytdl),
	YoutubeDL_getVideoID: ytdl.getVideoID.bind(ytdl)
};

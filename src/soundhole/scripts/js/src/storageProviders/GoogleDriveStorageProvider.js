
const { google } = require('googleapis');
const { v1: uuidv1 } = require('uuid');
const StorageProvider = require('./StorageProvider');

const PLAYLIST_IMAGE_DATA_KEY = 'SHPLAYLIST_IMAGE_DATA';
const PLAYLIST_IMAGE_MIMETYPE_KEY = 'SHPLAYLIST_IMAGE_MIMETYPE';


class GoogleDriveStorageProvider extends StorageProvider {
	constructor(options) {
		if(!options || typeof options !== 'object') {
			throw new Error("missing options");
		}
		if(!options.baseFolderName || typeof options.baseFolderName !== 'string') {
			throw new Error("missing options.baseFolderName");
		}
		if(!options.clientId || typeof options.clientId !== 'string') {
			throw new Error("missing options.clientId");
		}
		if(!options.clientSecret || typeof options.clientSecret !== 'string') {
			throw new Error("missing options.clientSecret");
		}

		this._options = options;
		this._baseFolderId = null;
		this._playlistsFolderId = null;
		
		this._auth = new google.auth.OAuth2({
			clientId: options.clientId,
			clientSecret: options.clientSecret
		});
		if(options.credentials) {
			this._auth.setCredentials(options.credentials);
		}
		
		this._drive = google.drive({
			version: 'v3',
			auth: this._auth
		});
		this._sheets = google.sheets({
			version: 'v4',
			auth: this._auth
		});
	}


	get name() {
		return 'googledrive';
	}

	get displayName() {
		return "Google Drive";
	}




	get canStorePlaylists() {
		return false;
	}

	async _prepareBaseFolder() {
		// if we have a base folder ID, we can stop
		if(this._baseFolderId != null) {
			return;
		}
		// check for name of base folder
		if(typeof this._options.baseFolderName !== 'string') {
			throw new Error("missing options.baseFolderName");
		}
		// check for existing base folder
		const folderName = options.baseFolderName;
		let baseFolder = (await this._drive.files.list({
			q: `mimeType = 'application/vnd.google-apps.folder' and name = '${folderName}' and 'appDataFolder' in parents and trashed = false`,
			fields: "nextPageToken, files(id, name)",
			spaces: 'drive',
			pageSize: 1
		})).files[0];
		if(baseFolder != null) {
			this._baseFolderId = baseFolder.id;
			return;
		}
		// create base folder
		baseFolder = await this._drive.files.create({
			name: folderName,
			parents: ['root'],
			media: {
				mimeType: 'application/vnd.google-apps.folder'
			}
		});
		this._baseFolderId = baseFolder.id;
	}

	async _preparePlaylistsFolder() {
		await this._prepareBaseFolder();
		// if we have a playlists folder ID, we can stop
		if(this._playlistsFolderId != null) {
			return;
		}
		// check for existing playlists folder
		const baseFolderId = this._baseFolderId;
		const folderName = 'playlists';
		let playlistsFolder = (await this._drive.files.list({
			q: `mimeType = 'application/vnd.google-apps.folder' and name = '${folderName}' and '${baseFolderId}' in parents and trashed = false`,
			fields: "nextPageToken, files(id, name)",
			spaces: 'appDataFolder',
			pageSize: 1
		})).files[0];
		if(playlistsFolder != null) {
			this._playlistsFolderId = playlistsFolder.id;
			return;
		}
		// create playlists folder
		playlistsFolder = await this._drive.files.create({
			name: folderName,
			parents: [baseFolderId],
			media: {
				mimeType: 'application/vnd.google-apps.folder'
			}
		});
		this._playlistsFolderId = playlistsFolder.id;
	}



	_createPlaylistObject(file) {
		// TODO add images, owner, privacy
		return {
			id: file.id,
			name: file.name,
			versionId: file.modifiedTime,
			description: file.description
		};
	}



	async createPlaylist(name, options={}) {
		// prepare folder
		await this._preparePlaylistsFolder();
		const playlistsFolderId = this._playlistsFolderId;
		if(playlistsFolderId == null) {
			throw new Error("missing playlists folder ID");
		}
		// prepare properties
		const properties = {};
		if(options.image) {
			properties[PLAYLIST_IMAGE_DATA_KEY] = options.image.data;
			properties[PLAYLIST_IMAGE_MIMETYPE_KEY] = options.image.mimeType;
		}
		// create file
		const file = await this._drive.files.create({
			name: name,
			description: options.description,
			parents: [playlistsFolderId],
			media: {
				mimeType: 'application/vnd.google-apps.spreadsheet'
			},
			properties: properties
		});
		// transform result
		return this._createPlaylistObject(file);
	}

	async getPlaylist(playlistId) {
		throw new Error("getPlaylist is not implemented");
	}

	async deletePlaylist(playlistId) {
		throw new Error("deletePlaylist is not implemented");
	}

	async getPlaylists(options={}) {
		throw new Error("getPlaylists is not implemented");
	}

	async getPlaylistItems(options) {
		throw new Error("getPlaylistItems is not implemented");
	}
}


module.exports = GoogleDriveStorageProvider;

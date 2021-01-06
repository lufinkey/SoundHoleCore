
const { google } = require('googleapis');
const { v1: uuidv1 } = require('uuid');
const StorageProvider = require('./StorageProvider');

const SOUNDHOLE_FOLDER_NAME = "My SoundHole";
const PLAYLISTS_FOLDER_NAME = "Playlists";
const SOUNDHOLE_BASE_KEY = 'SOUNDHOLE_BASE';
const PLAYLIST_KEY = 'SOUNDHOLE_PLAYLIST';
const PLAYLIST_IMAGE_DATA_KEY = 'SOUNDHOLEPLAYLIST_IMG_DATA';
const PLAYLIST_IMAGE_MIMETYPE_KEY = 'SOUNDHOLEPLAYLIST_IMG_MIMETYPE';


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
			clientSecret: options.clientSecret,
			redirectUri: options.redirectURL
		});
		if(options.apiKey) {
			this._auth.apiKey = options.apiKey;
		}
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

	get auth() {
		return this._auth;
	}

	get session() {
		const credentials = this._auth.credentials;
		if(!credentials) {
			return null;
		}
		if(!credentials.access_token || !credentials.expiry_date
		   || !credentials.token_type) {
			return null;
		}
		return credentials;
	}



	async loginWithRedirectParams(params, options) {
		if(params.error) {
			throw new Error(params.error);
		}
		if(!params.code) {
			throw new Error("Missing expected parameters in redirect URL");
		}
		const { tokens } = await this._auth.getToken({
			code: params.code,
			codeVerifier: options.codeVerifier,
			client_id: options.clientId
		});
		this._auth.setCredentials(tokens);
		try {
			await this._prepareFolders();
		} catch(error) {
			this._auth.revokeCredentials();
			throw error;
		}
	}

	logout() {
		this._auth.revokeCredentials();
	}

	

	async getCurrentUser() {
		if(this.session == null) {
			return null;
		}
		const about = await this._drive.about.get();
		if(!about.user) {
			return null;
		}
		return {
			...about.user,
			baseFolderId: this._baseFolderId
		};
	}



	get canStorePlaylists() {
		return true;
	}

	async _prepareFolders() {
		await this._prepareBaseFolder();
		await this._preparePlaylistsFolder();
	}

	async _prepareBaseFolder() {
		// if we have a base folder ID, we can stop
		if(this._baseFolderId != null) {
			if(this._baseFolder != null) {
				return;
			}
			const baseFolder = await this._drive.files.get({
				fileId: this._baseFolderId,
				fields: "*"
			});
			this._baseFolder = baseFolder;
			return;
		}
		// check for name of base folder
		if(typeof this._options.baseFolderName !== 'string') {
			throw new Error("missing options.baseFolderName");
		}
		// check for existing base folder
		const folderName = options.baseFolderName || SOUNDHOLE_FOLDER_NAME;
		let baseFolder = (await this._drive.files.list({
			q: `mimeType = 'application/vnd.google-apps.folder' and appProperties has {{ key='${SOUNDHOLE_BASE_KEY}' and value=true }} and trashed = false`,
			fields: "*",
			spaces: 'drive',
			pageSize: 1
		})).files[0];
		if(baseFolder != null) {
			this._baseFolderId = baseFolder.id;
			this._baseFolder = baseFolder;
			return;
		}
		// create base folder
		baseFolder = await this._drive.files.create({
			name: folderName,
			parents: ['root'],
			media: {
				mimeType: 'application/vnd.google-apps.folder'
			},
			appProperties: {
				[SOUNDHOLE_BASE_KEY]: true
			}
		});
		this._baseFolderId = baseFolder.id;
		this._baseFolder = baseFolder;
	}

	async _preparePlaylistsFolder() {
		await this._prepareBaseFolder();
		// if we have a playlists folder ID, we can stop
		if(this._playlistsFolderId != null) {
			return;
		}
		// check for existing playlists folder
		const baseFolderId = this._baseFolderId;
		const folderName = PLAYLISTS_FOLDER_NAME;
		let playlistsFolder = (await this._drive.files.list({
			q: `mimeType = 'application/vnd.google-apps.folder' and name = '${folderName}' and '${baseFolderId}' in parents and trashed = false`,
			fields: "nextPageToken, files(id, name)",
			spaces: 'drive',
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



	_parsePlaylistID(playlistId) {
		const index = playistId.indexOf('/');
		if(index === -1) {
			return {
				fileId: playlistId
			};
		}
		const baseFolderId = playlistId.substring(0, index);
		const fileId = playlistId.substring(index+1);
		return {
			fileId,
			baseFolderId
		};
	}

	_createPlaylistID(fileId, baseFolderId) {
		if(!fileId) {
			throw new Error("missing fileId for _createPlaylistID");
		} else if(!baseFolderId) {
			return fileId;
		}
		return `${fileId}/${baseFolderId}`;
	}

	_parseUserID(userId) {
		const index = userId.indexOf('/');
		if(index === -1) {
			if(userId.indexOf('@') !== -1) {
				return {
					email: userId
				};
			} else {
				return {
					permissionId: userId
				};
			}
		}
		const baseFolderId = userId.substring(0, index);
		const userIdPart = userId.substring(index+1);
		if(userIdPart.indexOf('@') !== -1) {
			return {
				email: userId,
				baseFolderId
			};
		} else {
			return {
				permissionId: userId,
				baseFolderId
			};
		}
	}

	_createUserID(identifier, baseFolderId) {
		if(!identifier) {
			throw new Error("missing identifier for _createUserID");
		} else if(!baseFolderId) {
			return identifier;
		}
		return `${identifier}/${baseFolderId}`;
	}

	_createUserObject(owner) {
		return {
			id: this._createUserID(owner.emailAddress || owner.permissionId || owner.id, baseFolderId),
			name: owner.displayName,
			imageURL: owner.photoLink
		}
	}

	_createPlaylistObject(file, userPermissionId, userFolderId) {
		// validate appProperties
		if(!file.appProperties[PLAYLIST_KEY] && !file.properties[PLAYLIST_KEY]) {
			throw new Error("file is not a SoundHole playlist");
		}
		// parse owner and base folder ID
		const owner = file.owners[0];
		let baseFolderId = null;
		if(userPermissionId && (owner.permissionId === userPermissionId || owner.emailAddress === userPermissionId)) {
			owner.baseFolderId = userFolderId;
			baseFolderId = userFolderId;
		} else if(file.ownedByMe && this._baseFolderId) {
			owner.baseFolderId = this._baseFolderId;
			baseFolderId = this._baseFolderId;
		}
		// parse privacy level
		let privacy = 'private';
		if(file.permissions instanceof Array && file.permissions.length > 0) {
			if(file.permissions.find((p) => (p.type === 'anyone')) != null) {
				privacy = 'public';
			}
		}
		// TODO parse images
		return {
			id: this._createPlaylistID(file.id, baseFolderId),
			name: file.name,
			versionId: file.modifiedTime,
			description: file.description,
			privacy: privacy,
			owner: owner ? this._createUserObject(owner) : null
		};
	}



	async createPlaylist(name, options={}) {
		// prepare folder
		await this._preparePlaylistsFolder();
		const baseFolder = this._baseFolder;
		if(baseFolder == null) {
			throw new Error("missing base folder");
		}
		const baseFolderOwner = baseFolder.owners[0];
		const playlistsFolderId = this._playlistsFolderId;
		if(playlistsFolderId == null) {
			throw new Error("missing playlists folder ID");
		}
		// prepare appProperties
		const appProperties = {};
		if(options.image) {
			appProperties[PLAYLIST_KEY] = true;
			appProperties[PLAYLIST_IMAGE_DATA_KEY] = options.image.data;
			appProperties[PLAYLIST_IMAGE_MIMETYPE_KEY] = options.image.mimeType;
		}
		// create file
		const file = await this._drive.files.create({
			name: name,
			description: options.description,
			parents: [playlistsFolderId],
			media: {
				mimeType: 'application/vnd.google-apps.spreadsheet'
			},
			appProperties: appProperties
		});
		// transform result
		return this._createPlaylistObject(file, (baseFolderOwner ? baseFolderOwner.permissionId : null), baseFolder.id);
	}


	async getPlaylist(playlistId) {
		const idParts = this._parsePlaylistID(playlistId);
		// get file
		const file = await this._drive.files.get({
			fileId: idParts.fileId,
			fields: "*"
		});
		// transform result
		return this._createPlaylistObject(file, idParts.permissionId || idParts.email, idParts.baseFolderId);
	}


	async deletePlaylist(playlistId, options={}) {
		const idParts = this._parsePlaylistID(playlistId);
		if(options.permanent) {
			// delete file
			await this._drive.files.delete(idParts.fileId);
		} else {
			// move file to trash
			await this._drive.files.update(idParts.fileId, {trashed:true});
		}
	}


	async getMyPlaylists(options={}) {
		// prepare playlists folder
		await this._preparePlaylistsFolder();
		const baseFolder = this._baseFolder;
		if(baseFolder == null) {
			throw new Error("missing base folder");
		}
		const baseFolderOwner = baseFolder.owners[0];
		const playlistsFolderId = this._playlistsFolderId;
		if(playlistsFolderId == null) {
			throw new Error("missing playlists folder ID");
		}
		// get files
		const { files, nextPageToken } = await this._drive.files.list({
			q: `mimeType = 'application/vnd.google-apps.spreadsheet' and '${playlistsFolderId}' in parents and trashed = false`,
			fields: "*",
			pageToken: options.pageToken
		});
		// transform result
		return {
			items: (files || []).map((file) => (
				this._createPlaylistObject(file, (baseFolderOwner ? baseFolderOwner.permissionId : null), baseFolder.id)
			)),
			nextPageToken
		};
	}


	async getUserPlaylists(userId, options={}) {
		const idParts = this._parseUserID(userId);
		if(!idParts.baseFolderId) {
			throw new Error("user ID does not include SoundHole folder ID");
		}
		// parse page token if available
		let playlistsFolderId = null;
		let pageToken = undefined;
		if(options.pageToken) {
			const index = options.pageToken.indexOf(':');
			if(index === -1) {
				throw new Error("invalid page token");
			}
			playlistsFolderId = options.pageToken.substring(0, index);
			pageToken = options.pageToken.substring(index+1);
		}
		// get playlists folder if needed
		if(!playlistsFolderId) {
			const folderName = PLAYLISTS_FOLDER_NAME;
			const playlistsFolder = (await this._drive.files.list({
				q: `mimeType = 'application/vnd.google-apps.folder' and name = '${folderName}' and '${idParts.baseFolderId}' in parents and trashed = false`,
				fields: "*",
				pageSize: 1
			})).files[0];
			if(!playlistsFolder) {
				return {
					items: [],
					nextPageToken: null
				};
			}
			playlistsFolderId = playlistsFolder.id;
		}
		// get files in playlists folder
		const { files, nextPageToken } = await this._drive.files.list({
			q: `mimeType = 'application/vnd.google-apps.spreadsheet' and '${playlistsFolderId}' in parents and trashed = false`,
			fields: "*",
			pageToken: pageToken
		});
		// transform result
		return {
			items: (files || []).map((file) => (
				this._createPlaylistObject(file, (baseFolderOwner ? baseFolderOwner.permissionId : null), baseFolder.id)
			)),
			nextPageToken: (nextPageToken ? `${playlistsFolderid}:${nextPageToken}` : null)
		};
	}


	async getPlaylistItems(options) {
		throw new Error("getPlaylistItems is not implemented");
	}
}


module.exports = GoogleDriveStorageProvider;

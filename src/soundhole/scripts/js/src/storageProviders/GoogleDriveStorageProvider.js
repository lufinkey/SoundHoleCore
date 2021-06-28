
const { google } = require('googleapis');
const { v1: uuidv1 } = require('uuid');
const StorageProvider = require('./StorageProvider');

const PLAYLISTS_FOLDER_NAME = "Playlists";
const MIN_PLAYLIST_COLUMNS = [
	'uri',
	'provider',
	'name',
	'albumName',
	'albumURI',
	'artists',
	'images',
	'duration',
	'addedAt',
	'addedBy'
];
const PLAYLIST_COLUMNS = [
	'uri',
	'provider',
	'name',
	'albumName',
	'albumURI',
	'artists',
	'images',
	'duration',
	'addedAt',
	'addedBy',
	'addedById'
];
const PLAYLIST_ITEM_ONLY_COLUMNS = [
	'addedAt',
	'addedBy',
	'addedById'
];
const PLAYLIST_VERSIONS = [ 'v1.0' ];
const PLAYLIST_LATEST_VERSION = 'v1.0';
const PLAYLIST_ITEMS_START_OFFSET = 2;
const PLAYLIST_PRIVACIES = ['public','unlisted','private'];

const PLAYLIST_EXTRA_COLUMN_COUNT = 10;


class GoogleDriveStorageProvider extends StorageProvider {
	constructor(options) {
		super();
		if(!options || typeof options !== 'object') {
			throw new Error("missing options");
		}
		if(!options.appKey || typeof options.appKey !== 'string') {
			throw new Error("missing options.appKey");
		}
		if(!options.baseFolderName || typeof options.baseFolderName !== 'string') {
			throw new Error("missing options.baseFolderName");
		}
		if(!options.clientId || typeof options.clientId !== 'string') {
			throw new Error("missing options.clientId");
		}

		this._options = options;
		this._baseFolderId = null;
		this._playlistsFolderId = null;
		this._currentDriveInfo = null;
		
		this._auth = new google.auth.OAuth2({
			clientId: options.clientId,
			clientSecret: options.clientSecret,
			redirectUri: options.redirectURL
		});
		if(options.apiKey) {
			this._auth.apiKey = options.apiKey;
		}
		if(options.session) {
			this._auth.setCredentials(options.session);
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
			throw new Error(`Error logging in from redirect: ${params.error}`);
		}
		if(!params.code) {
			throw new Error("Missing expected parameters in redirect URL");
		}
		const { tokens } = await this._auth.getToken({
			code: params.code,
			codeVerifier: options.codeVerifier,
			client_id: options.clientId || this._options.clientId
		});
		this._auth.setCredentials(tokens);
		try {
			await this._prepareFolders();
			const baseFolderId = this._baseFolderId;
			if(!baseFolderId) {
				throw new Error("could not find base folder id");
			}
			const about = (await this._drive.about.get({
				fields: "*"
			})).data;
			if(!about.user) {
				throw new Error("unable to fetch current user data");
			}
			this._currentDriveInfo = about;
		} catch(error) {
			this._auth.revokeCredentials();
			throw error;
		}
	}

	logout() {
		if(this._auth.credentials != null && this._auth.credentials.access_token != null) {
			this._auth.revokeCredentials()
				.catch((error) => {
					console.error(error);
				});
		}
		this._currentDriveInfo = null;
	}

	
	
	async _getCurrentDriveInfo() {
		if(this.session == null) {
			throw new Error("Google Drive is not logged in");
		}
		if(this._currentDriveInfo != null) {
			return this._currentDriveInfo;
		}
		const about = (await this._drive.about.get({
			fields: "*"
		})).data;
		if(!about.user) {
			throw new Error("Unable to fetch current user data");
		}
		if(this.session == null) {
			throw new Error("Google Drive is not logged in");
		}
		this._currentDriveInfo = about;
		return about;
	}

	async getCurrentUser() {
		if(this.session == null) {
			return null;
		}
		const about = (await this._drive.about.get({
			fields: "*"
		})).data;
		if(!about.user) {
			return null;
		}
		this._currentDriveInfo = about;
		return {
			...about.user,
			uri: this._createUserURIFromUser(about.user, this._baseFolderId),
			baseFolderId: this._baseFolderId
		};
	}



	get _baseFolderPropKey() {
		return `${this._options.appKey}_base_folder`;
	}
	get _playlistPropKey() {
		return `${this._options.appKey}_playlist`;
	}
	get _imageDataPropKey() {
		return `${this._options.appKey}_img_data`;
	}
	get _imageMimeTypePropKey() {
		return `${this._options.appKey}_img_mimetype`;
	}
	get _playlistItemIdKey() {
		return `${this._options.appKey}_playlist_item_id`;
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
			const baseFolder = (await this._drive.files.get({
				fileId: this._baseFolderId,
				fields: "*"
			})).data;
			this._baseFolder = baseFolder;
			return;
		}
		// check for name of base folder
		if(typeof this._options.baseFolderName !== 'string') {
			throw new Error("missing options.baseFolderName");
		}
		// check for existing base folder
		const folderName = this._options.baseFolderName;
		const baseFolderPropKey = this._baseFolderPropKey;
		let baseFolder = (await this._drive.files.list({
			q: `mimeType = 'application/vnd.google-apps.folder' and appProperties has { key='${baseFolderPropKey}' and value='true' } and trashed = false`,
			fields: "*",
			spaces: 'drive',
			pageSize: 1
		})).data.files[0];
		if(baseFolder != null) {
			this._baseFolderId = baseFolder.id;
			this._baseFolder = baseFolder;
			return;
		}
		// create base folder
		baseFolder = (await this._drive.files.create({
			fields: "*",
			requestBody: {
				name: folderName,
				mimeType: 'application/vnd.google-apps.folder',
				parents: [ 'root' ],
				appProperties: {
					[baseFolderPropKey]: 'true'
				}
			}
		})).data;
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
		})).data.files[0];
		if(playlistsFolder != null) {
			this._playlistsFolderId = playlistsFolder.id;
			return;
		}
		// create playlists folder
		playlistsFolder = (await this._drive.files.create({
			fields: "*",
			requestBody: {
				name: folderName,
				mimeType: 'application/vnd.google-apps.folder',
				parents: [ baseFolderId ],
			}
		})).data;
		this._playlistsFolderId = playlistsFolder.id;
	}



	_parseURI(uri) {
		if(!this._options.mediaItemBuilder || !this._options.mediaItemBuilder.parseStorageProviderURI) {
			const colonIndex1 = uri.indexOf(':');
			if(colonIndex1 === -1) {
				throw new Error("invalid URI "+uri);
			}
			const provider = uri.substring(0, colonIndex);
			if(provider !== this.name) {
				throw new Error("URI provider "+provider+" does not match expected provider name "+this.name);
			}
			const colonIndex2 = uri.indexOf(':', colonIndex1 + 1);
			if(colonIndex2 !== -1) {
				throw new Error("Invalid URI "+uri);
			}
			const type = uri.substring(colonIndex1 + 1, colonIndex2);
			const id = uri.substring(colonIndex2+1);
			return {
				storageProvider: provider,
				type,
				id
			};
		}
		return this._options.mediaItemBuilder.parseStorageProviderURI(uri);
	}

	_createURI(type, id) {
		if(!this._options.mediaItemBuilder || !this._options.mediaItemBuilder.createStorageProviderURI) {
			return `${this.name}:${type}:${id}`;
		}
		return this._options.mediaItemBuilder.createStorageProviderURI(this.name,type,id);
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
		return `${baseFolderId}/${fileId}`;
	}



	_parsePlaylistURI(playlistURI) {
		const uriParts = this._parseURI(playlistURI);
		const idParts = this._parsePlaylistID(uriParts.id);
		delete uriParts.id;
		return {
			...uriParts,
			...idParts
		};
	}

	_createPlaylistURI(fileId, baseFolderId) {
		return this._createURI('playlist', this._createPlaylistID(fileId, baseFolderId));
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
		return `${baseFolderId}/${identifier}`;
	}

	_createUserIDFromUser(user, baseFolderId) {
		return this._createUserID(user.emailAddress || user.permissionId, baseFolderId);
	}



	_parseUserURI(userURI) {
		const uriParts = this._parseURI(userURI);
		const idParts = this._parsePlaylistID(uriParts.id);
		delete uriParts.id;
		return {
			...uriParts,
			...idParts
		};
	}

	_createUserURI(identifier, baseFolderId) {
		return this._createURI('user', this._createUserID(identifier, baseFolderId));
	}

	_createUserURIFromUser(user, baseFolderId) {
		return this._createURI('user', this._createUserIDFromUser(user, baseFolderId));
	}



	_createUserObject(owner, baseFolderId) {
		const mediaItemBuilder = this._options.mediaItemBuilder;
		return {
			partial: false,
			type: 'user',
			provider: (mediaItemBuilder != null) ? mediaItemBuilder.name : this.name,
			uri: this._createUserURIFromUser(owner, baseFolderId),
			name: owner.displayName || owner.emailAddress || owner.permissionId,
			images: (owner.photoLink != null) ? [
				{
					url: owner.photoLink,
					size: 'MEDIUM'
				}
			] : []
		};
	}



	_createPlaylistObject(file, userPermissionId, userFolderId) {
		// validate appProperties
		const playlistPropKey = this._playlistPropKey;
		if(file.appProperties[playlistPropKey] != 'true' && file.properties[playlistPropKey] != 'true') {
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
		// TODO parse images
		return {
			partial: false,
			type: 'playlist',
			uri: this._createPlaylistURI(file.id, baseFolderId),
			name: file.name,
			description: file.description,
			versionId: file.modifiedTime,
			privacy: this._determinePrivacy(file.permissions),
			owner: owner ? this._createUserObject(owner, baseFolderId) : null
		};
	}



	_validatePrivacy(privacy) {
		if(privacy && PLAYLIST_PRIVACIES.indexOf(privacy) === -1) {
			throw new Error(`invalid privacy value "${privacy}" for google drive storage provider`);
		}
	}

	_determinePrivacy(permissions) {
		let privacy = 'private';
		for(const p of permissions) {
			if(p.type === 'anyone') {
				if(p.allowFileDiscovery) {
					privacy = 'public';
					break;
				} else {
					privacy = 'unlisted';
				}
			}
		}
		return privacy;
	}



	async createPlaylist(name, options={}) {
		// validate input
		this._validatePrivacy(options.privacy);
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
		appProperties[this._playlistPropKey] = 'true';
		if(options.image) {
			appProperties[this._imageDataPropKey] = options.image.data;
			appProperties[this._imageMimeTypePropKey] = options.image.mimeType;
		}
		// create file
		const file = (await this._drive.files.create({
			fields: "*",
			requestBody: {
				name: name,
				description: options.description,
				mimeType: 'application/vnd.google-apps.spreadsheet',
				parents: [playlistsFolderId],
				appProperties: appProperties
			}
		})).data;
		// update sheet
		await this._preparePlaylistSheet(file.id);
		// add privacy if necessary
		if(options.privacy) {
			if(options.privacy === 'public') {
				await this._drive.permissions.create({
					fileId: file.id,
					requestBody: {
						type: 'anyone',
						role: 'reader',
						allowFileDiscovery: true
					}
				});
			} else if(options.privacy === 'unlisted') {
				await this._drive.permissions.create({
					fileId: file.id,
					requestBody: {
						type: 'anyone',
						role: 'reader',
						allowFileDiscovery: false
					}
				});
			} else if(options.privacy === 'private') {
				// no need to add any extra permissions
			} else {
				throw new Error(`unsupported privacy value "${options.privacy}"`);
			}
		}
		// transform result
		const playlist = this._createPlaylistObject(file, (baseFolderOwner ? baseFolderOwner.permissionId : null), baseFolder.id);
		playlist.itemCount = 0;
		playlist.items = [];
		return playlist;
	}


	async getPlaylist(playlistURI, options) {
		// validate options
		if(options.itemsStartIndex != null && (!Number.isInteger(options.itemsStartIndex) || options.itemsStartIndex < 0)) {
			throw new Error("options.itemsStartIndex must be a positive integer");
		}
		if(options.itemsLimit != null && (!Number.isInteger(options.itemsLimit) || options.itemsLimit < 0)) {
			throw new Error("options.itemsLimit must be a positive integer");
		}
		// parse uri
		const uriParts = this._parsePlaylistURI(playlistURI);
		// get file
		const filePromise = (this._drive.files.get({
			fileId: uriParts.fileId,
			fields: "*"
		})).data;
		// get spreadsheet data if needed
		const sheetPromise = (Number.isInteger(options.itemsStartIndex) || Number.isInteger(options.itemsLimit)) ?
			this._sheets.spreadsheets.get({
				spreadsheetId: uriParts.fileId,
				includeGridData: true,
				ranges: [
					this._playlistHeaderA1Range(),
					this._playlistItemsA1Range(options.itemsStartIndex || 0, options.itemsLimit || 100)
				]
			})
			: Promise.resolve({data: null});
		// await results
		const [ {data: file}, {data: spreadsheet} ] = await Promise.all([ filePromise, sheetPromise ]);
		// transform result
		const playlist = this._createPlaylistObject(file, uriParts.permissionId || uriParts.email, uriParts.baseFolderId);
		const sheetProps = this._parsePlaylistSheetProperties(spreadsheet, 0);
		const tracks = this._parsePlaylistSheetItems(spreadsheet, 1, sheetProps);
		if(tracks && tracks.total != null) {
			playlist.itemCount = tracks.total;
		}
		const playlistItems = {};
		if(tracks && tracks.offset != null) {
			let i = tracks.offset;
			for(const item of tracks.items) {
				playlistItems[i] = item;
				i++;
			}
		}
		playlist.items = playlistItems;
		return playlist;
	}


	async deletePlaylist(playlistURI, options={}) {
		const uriParts = this._parsePlaylistURI(playlistURI);
		if(options.permanent) {
			// delete file
			await this._drive.files.delete(uriParts.fileId);
		} else {
			// move file to trash
			await this._drive.files.update(uriParts.fileId, {
				requestBody: {
					trashed: true
				}
			});
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
		const { files, nextPageToken } = (await this._drive.files.list({
			q: `mimeType = 'application/vnd.google-apps.spreadsheet' and '${playlistsFolderId}' in parents and trashed = false`,
			fields: "*",
			pageToken: options.pageToken
		})).data;
		// transform result
		return {
			items: (files || []).map((file) => (
				this._createPlaylistObject(file, (baseFolderOwner ? baseFolderOwner.permissionId : null), baseFolder.id)
			)),
			nextPageToken
		};
	}


	async getUserPlaylists(userURI, options={}) {
		const uriParts = this._parseUserURI(userURI);
		if(!uriParts.baseFolderId) {
			throw new Error("user URI does not include SoundHole folder ID");
		}
		const baseFolderId = uriParts.baseFolderId;
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
				q: `mimeType = 'application/vnd.google-apps.folder' and name = '${folderName}' and '${baseFolderId}' in parents and trashed = false`,
				fields: "*",
				pageSize: 1
			})).data.files[0];
			if(!playlistsFolder) {
				return {
					items: [],
					nextPageToken: null
				};
			}
			playlistsFolderId = playlistsFolder.id;
		}
		// get files in playlists folder
		const { files, nextPageToken } = (await this._drive.files.list({
			q: `mimeType = 'application/vnd.google-apps.spreadsheet' and '${playlistsFolderId}' in parents and trashed = false`,
			fields: "*",
			pageToken: pageToken
		})).data;
		// transform result
		return {
			items: (files || []).map((file) => (
				this._createPlaylistObject(file, (uriParts.permissionId || uriParts.email), baseFolderId)
			)),
			nextPageToken: (nextPageToken ? `${playlistsFolderid}:${nextPageToken}` : null)
		};
	}



	async isPlaylistEditable(playlistURI) {
		const driveInfo = await this._getCurrentDriveInfo();
		const uriParts = this._parsePlaylistURI(playlistURI);
		let done = false;
		let pageToken = undefined;
		const editRoles = ['writer','fileOrganizer','organizer','owner'];
		while(!done) {
			const { permissions, nextPageToken } = (await this._drive.permissions.list({
				fileId: uriParts.fileId,
				pageSize: 100,
				pageToken
			})).data;
			for(const p of permissions) {
				if(editRoles.indexOf(p.role) === -1) {
					continue;
				}
				if(p.type === 'anyone') {
					return true;
				}
				else if(p.type === 'user' && driveInfo.user && p.emailAddress === driveInfo.user.emailAddress) {
					return true;
				}
			}
			pageToken = nextPageToken;
			if(!pageToken) {
				done = true;
			}
		}
		return false;
	}



	_indexToColumn(index) {
		let letter = '', temp;
		while(index >= 0) {
			temp = index % 26;
			letter = String.fromCharCode(temp + 65) + letter;
			index = (index - temp - 1) / 26;
		}
		return letter;
	}
	_a1Notation(x,y) {
		return ''+this._indexToColumn(x)+(y+1);
	}
	_a1Range(x1,y1,x2,y2) {
		return this._a1Notation(x1,y1)+':'+this._a1Notation(x2,y2)
	}
	_playlistHeaderA1Range() {
		return this._a1Range(0,0, (PLAYLIST_COLUMNS.length+PLAYLIST_EXTRA_COLUMN_COUNT), 1);
	}
	_playlistItemsA1Range(startIndex, itemCount) {
		if(!Number.isInteger(startIndex) || startIndex < 0) {
			throw new Error("startIndex must be a positive integer");
		} else if(!Number.isInteger(itemCount) || itemCount <= 0) {
			throw new Error("itemCount must be a positive non-zero integer");
		}
		return this._a1Range(0,(PLAYLIST_ITEMS_START_OFFSET+startIndex), (PLAYLIST_COLUMNS.length+PLAYLIST_EXTRA_COLUMN_COUNT), (PLAYLIST_ITEMS_START_OFFSET+itemCount));
	}
	_playlistColumnsA1Range() {
		return this._a1Range(0,1, PLAYLIST_COLUMNS.length,1);
	}

	_parseUserEnteredValue(val) {
		if(!val) {
			return undefined;
		} else if(val.stringValue != null) {
			return val.stringValue;
		} else if(val.numberValue != null) {
			return ''+val.numberValue;
		} else if(val.boolValue != null) {
			return ''+val.boolValue;
		} else if(val.formulaValue != null) {
			return ''+val.formulaValue;
		} else if(val.errorValue != null) {
			return `Error(type=${JSON.stringify(val.errorValue.type)}, message=${JSON.stringify(val.errorValue.message)})`;
		} else {
			return undefined;
		}
	}

	_formatDate(date) {
		const fixedDate = new Date(Math.floor(date.getTime() / 1000) * 1000);
		let dateString = fixedDate.toISOString();
		const suffix = ".000Z";
		if(dateString.endsWith(suffix)) {
			dateString = dateString.substring(0,dateString.length-suffix.length)+'Z';
		}
		return dateString;
	}

	async _preparePlaylistSheet(fileId) {
		// get spreadsheet header
		let spreadsheet = (await this._sheets.spreadsheets.get({
			spreadsheetId: fileId,
			includeGridData: true,
			ranges: [ this._playlistHeaderA1Range() ]
		})).data;
		// validate properties
		try {
			this._parsePlaylistSheetProperties(spreadsheet, 0);
			// properties are valid, we don't need to continue with preparing the playlist
			return;
		} catch(error) {
			// continue on preparing the sheet
		}
		// get sheet
		if(!spreadsheet || !spreadsheet.sheets || !spreadsheet.sheets[0]) {
			throw new Error(`Missing spreadsheet sheets`);
		}
		const sheet = spreadsheet.sheets[0];
		if(!sheet.properties) {
			throw new Error(`Missing spreadsheet properties`);
		}
		const gridProps = sheet.properties.gridProperties;
		const maxColumnsCount = PLAYLIST_COLUMNS.length;
		const maxRowsCount = PLAYLIST_ITEMS_START_OFFSET;
		const rowDiff = maxColumnsCount - gridProps.columnCount;
		const colDiff = maxRowsCount - gridProps.rowCount;
		// perform batch update
		await this._sheets.spreadsheets.batchUpdate({
			spreadsheetId: fileId,
			requestBody: {
				requests: [
					// resize columns
					...((colDiff > 0) ? [
						{appendDimension: {
							sheetId: 0,
							dimension: 'COLUMNS',
							length: colDiff
						}}
					] : (colDiff < 0) ? [
						{deleteDimension: {
							range: {
								sheetId: 0,
								dimension: 'COLUMNS',
								startIndex: maxColumnsCount,
								endIndex: gridProps.columnCount
							}
						}}
					] : []),
					// resize rows
					...((rowDiff > 0) ? [
						{appendDimension: {
							sheetId: 0,
							dimension: 'ROWS',
							length: rowDiff
						}}
					] : (rowDiff < 0) ? [
						{deleteDimension: {
							range: {
								sheetId: 0,
								dimension: 'ROWS',
								startIndex: maxRowsCount,
								endIndex: gridProps.rowCount
							}
						}}
					] : []),
					// update version, add columns
					{updateCells: {
						fields: 'userEnteredValue',
						start: {
							sheetId: 0,
							rowIndex: 0,
							columnIndex: 0
						},
						rows: [
							{values: [
								{userEnteredValue: {stringValue: PLAYLIST_LATEST_VERSION}}
							]},
							{values: PLAYLIST_COLUMNS.map((column) => (
								{userEnteredValue: {stringValue: column}}
							))}
						]
					}}
				]
			}
		});
	}

	_parsePlaylistSheetProperties(spreadsheet, rangeIndex) {
		// get sheet data
		if(!spreadsheet || !spreadsheet.sheets || !spreadsheet.sheets[0]) {
			throw new Error(`Missing spreadsheet sheets`);
		}
		const sheet = spreadsheet.sheets[0];
		if(!sheet.data || !sheet.data[rangeIndex] || !sheet.data[rangeIndex].rowData) {
			throw new Error(`Missing spreadsheet data`);
		}
		const gridProps = sheet.properties.gridProperties;
		// calculate item count
		const itemsStartOffset = PLAYLIST_ITEMS_START_OFFSET;
		const itemCount = gridProps.rowCount - itemsStartOffset;
		// parse top row properties
		let version = null;
		const data = sheet.data[rangeIndex];
		if(data.rowData[0]) {
			const rowDataValues = data.rowData[0].values;
			if(rowDataValues[0]) {
				version = this._parseUserEnteredValue(rowDataValues[0].userEnteredValue);
			}
		}
		// parse columns
		let columns = [];
		if(data.rowData[1]) {
			for(const cell of data.rowData[1].values) {
				const cellValue = this._parseUserEnteredValue(cell.userEnteredValue);
				if(cellValue != null && cellValue !== '') {
					columns.push(`${cellValue}`);
				} else {
					break;
				}
			}
		}
		// validate version
		if(PLAYLIST_VERSIONS.indexOf(version) === -1) {
			throw new Error(`Invalid playlist version '${version}'`);
		}
		// ensure we have columns
		if(columns.length === 0) {
			throw new Error(`Missing track columns`);
		}
		// check for minimum necessary columns
		for(const column of MIN_PLAYLIST_COLUMNS) {
			if(columns.indexOf(column) === -1) {
				throw new Error(`Missing track column ${column}`);
			}
		}
		return { columns, itemCount, itemsStartRowIndex: itemsStartOffset };
	}

	_parsePlaylistSheetItems(spreadsheet, rangeIndex, { columns, itemCount, itemsStartRowIndex }) {
		// parse out sheet data
		if(!spreadsheet || !spreadsheet.sheets || !spreadsheet.sheets[0]) {
			throw new Error(`Missing spreadsheet sheets`);
		}
		const sheet = spreadsheet.sheets[0];
		if(!sheet.data || !sheet.data[rangeIndex] || !sheet.data[rangeIndex].rowData) {
			throw new Error(`Missing spreadsheet data`);
		}
		const data = sheet.data[rangeIndex];
		// parse tracks
		const startIndex = data.startRow - itemsStartRowIndex;
		const items = [];
		let i = 0;
		for(const row of data.rowData) {
			const rowMetadata = data.rowMetadata[i];
			const index = startIndex + items.length;
			if(index >= itemCount) {
				break;
			}
			const item = this._parsePlaylistSheetItemRow(row, rowMetadata, columns, index);
			items.push(item);
			i++;
		}
		return {
			offset: startIndex,
			total: itemCount,
			items
		};
	}

	_parsePlaylistSheetItemRow(row, rowMetadata, columns, index) {
		if(row.values.length < columns.length) {
			throw new Error("Not enough columns in row");
		}
		const rowItemIdKey = this._playlistItemIdKey;
		const item = {};
		const track = {};
		for(let i=0; i<columns.length; i++) {
			const columnName = columns[i];
			const cell = row.values[i];
			const value = JSON.parse(this._parseUserEnteredValue(cell.userEnteredValue));
			if(PLAYLIST_ITEM_ONLY_COLUMNS.indexOf(columnName) !== -1) {
				item[columnName] = value;
			} else {
				track[columnName] = value;
			}
			const itemIdMetadata = rowMetadata.developerMetadata.find((metadata) => {
				return metadata.metadataKey === rowItemIdKey
			});
			if(!itemIdMetadata) {
				throw new Error(`Missing developer metadata '${rowItemIdKey}' for track at index ${index}`);
			}
			item.uniqueId = itemIdMetadata.metadataValue;
		}
		item.track = track;
		return item;
	}


	async getPlaylistItems(playlistURI, { offset, limit }) {
		if(!Number.isInteger(offset) || offset < 0) {
			throw new Error("offset must be a positive integer");
		} else if(!Number.isInteger(limit) || limit <= 0) {
			throw new Error("limit must be a positive non-zero integer");
		}
		// parse uri
		const uriParts = this._parsePlaylistURI(playlistURI);
		// get spreadsheet data
		const spreadsheet = (await this._sheets.spreadsheets.get({
			spreadsheetId: uriParts.fileId,
			includeGridData: true,
			ranges: [
				this._playlistHeaderA1Range(),
				this._playlistItemsA1Range(options.offset, options.limit)
			]
		})).data;
		// parse spreadsheet data
		const sheetProps = this._parsePlaylistSheetProperties(spreadsheet, 0);
		const itemsPage = this._parsePlaylistSheetItems(spreadsheet, 1, sheetProps);
		return itemsPage;
	}



	async _insertPlaylistItems(playlistURI, index, tracks, sheetProps, driveInfo) {
		const rowItemIdKey = this._playlistItemIdKey;
		// parse uri
		const uriParts = this._parsePlaylistURI(playlistURI);
		// build items data
		const addedAt = this._formatDate(new Date());
		const addedBy = this._createUserObject(driveInfo.user, this._baseFolderId);
		const addedById = `/${this._createUserIDFromUser(driveInfo.user, this._baseFolderId)}/`;
		const items = tracks.map((track) => (
			{
				uniqueId: uuidv1(),
				addedAt,
				addedBy,
				addedById,
				track
			}
		));
		// insert tracks into playlist
		await this._sheets.spreadsheets.batchUpdate({
			spreadsheetId: uriParts.fileId,
			requestBody: {
				requests: [
					// insert empty rows
					{insertDimension: {
						inheritFromBefore: true,
						range: {
							dimension: 'ROWS',
							startIndex: PLAYLIST_ITEMS_START_OFFSET + index,
							endIndex:  PLAYLIST_ITEMS_START_OFFSET + index + tracks.length
						}
					}},
					// add content to new rows
					{updateCells: {
						fields: 'userEnteredValue',
						start: {
							sheetId: 0,
							rowIndex:  PLAYLIST_ITEMS_START_OFFSET + index,
							columnIndex: 0
						},
						rows: items.map((item) => {
							const track = item.track;
							const rowValues = [];
							for(const columnName of sheetProps.columns) {
								let trackProp;
								if(PLAYLIST_ITEM_ONLY_COLUMNS.indexOf(columnName) === -1) {
									trackProp = item[columnName];
								} else {
									trackProp = track[columnName];
								}
								rowValues.push({
									userEnteredValue: {
										stringValue: JSON.stringify(trackProp)
									}
								});
							}
							return { values: rowValues };
						})
					}},
					// add developer metadata for tracks
					...items.map((item, i) => (
						{createDeveloperMetadata: {
							developerMetadata: {
								// item ID
								metadataKey: rowItemIdKey,
								metadataValue: item.uniqueId,
								location: {
									dimensionRange: {
										dimension: 'ROWS',
										sheetId: 0,
										startIndex: PLAYLIST_ITEMS_START_OFFSET + index + i,
										endIndex: PLAYLIST_ITEMS_START_OFFSET + index + (i+1)
									}
								},
								visibility: 'DOCUMENT'
							}
						}}
					))
				]
			}
		});
		// return resulting tracks
		return {
			offset: index,
			total: (sheetProps.itemCount + items.length),
			items
		};
	}

	async insertPlaylistItems(playlistURI, index, tracks) {
		if(!Number.isInteger(index) || index < 0) {
			throw new Error("index must be a positive integer");
		}
		// parse uri
		const uriParts = this._parsePlaylistURI(playlistURI);
		// get drive info
		const driveInfo = await this._getCurrentDriveInfo();
		// get spreadsheet info
		const spreadsheet = (await this._sheets.spreadsheets.get({
			spreadsheetId: uriParts.fileId,
			includeGridData: true,
			ranges: [
				this._playlistHeaderA1Range()
			]
		})).data;
		const sheetProps = this._parsePlaylistSheetProperties(spreadsheet, 0);
		// insert new rows
		return await this._insertPlaylistItems(playlistURI, index, tracks, sheetProps, driveInfo);
	}

	async appendPlaylistItems(playlistURI, tracks) {
		// parse uri
		const uriParts = this._parsePlaylistURI(playlistURI);
		// get drive info
		const driveInfo = await this._getCurrentDriveInfo();
		// get spreadsheet info
		const spreadsheet = (await this._sheets.spreadsheets.get({
			spreadsheetId: uriParts.fileId,
			includeGridData: true,
			ranges: [
				this._playlistHeaderA1Range()
			]
		})).data;
		const sheetProps = this._parsePlaylistSheetProperties(spreadsheet, 0);
		// append new rows
		return await this._insertPlaylistItems(playlistURI, sheetProps.itemCount, tracks, sheetProps, driveInfo);
	}

	async deletePlaylistItems(playlistURI, itemIds) {
		if(itemIds.length === 0) {
			return;
		}
		const rowItemIdKey = this._playlistItemIdKey;
		// parse uri
		const uriParts = this._parsePlaylistURI(playlistURI);
		// find matching item ids
		const { matchedDeveloperMetadata } = (await this._sheets.spreadsheets.developerMetadata.search({
			spreadsheetId: uriParts.fileId,
			requestBody: {
				dataFilters: itemIds.map((itemId) => ({
					developerMetadata: {
						metadataKey: rowItemIdKey,
						metadataValue: itemId
					}
				}))
			}
		})).data;
		// get row indexes
		const rows = itemIds.map((itemId) => {
			const metadata = matchedDeveloperMetadata.find((metadata) => {
				const devMeta = metadata.developerMetadata;
				return (devMeta.metadataKey === rowItemIdKey && devMeta.metadataValue == itemId);
			});
			if(!metadata) {
				throw new Error(`Could not find playlist item with id ${itemId}`);
			}
			return {
				itemId,
				rowIndex: metadata.developerMetadata.location.dimensionRange.startIndex
			};
		});
		rows.sort((a, b) => (a.index - b.index));
		// group indexes for simpler removal
		const rowGroups = [];
		let groupStartIndex = null;
		let expectedNextIndex = null;
		for(const { rowIndex } of rows) {
			if(groupStartIndex == null) {
				// start a new group
				groupStartIndex = rowIndex;
				expectedNextIndex = rowIndex + 1;
			} else if(rowIndex === expectedNextIndex) {
				// found expected index, expand group
				expectedNextIndex = rowIndex + 1;
			} else {
				// next index wasn't the expected index, so push the group
				rowGroups.push({
					startIndex: groupStartIndex,
					endIndex: expectedNextIndex
				});
				// start a new group
				groupStartIndex = rowIndex;
				expectedNextIndex = rowindex + 1;
			}
		}
		// add the last group
		if(groupStartIndex != null) {
			rowGroups.push({
				startIndex: groupStartIndex,
				endIndex: expectedNextIndex
			});
		}
		// delete from last to first so indexes are correct
		rowGroups.reverse();
		// delete row index groups
		await this._sheets.spreadsheets.batchUpdate({
			spreadsheetId: uriParts.fileId,
			requestBody: {
				requests: [
					// validate the item IDs at the indexes by setting them to the same values
					...rows.map(({ itemId, rowIndex }) => (
						{updateDeveloperMetadata: {
							dataFilters: [
								{developerMetadataLookup: {
									locationType: 'ROW',
									metadataLocation: {
										locationType: 'ROW',
										dimensionsRange: {
											sheetId: 0,
											startIndex: rowIndex,
											endIndex: rowIndex + 1,
											dimension: 'ROWS'
										}
									},
									locationMatchingStrategy: 'EXACT',
									metadataKey: rowItemIdKey,
									metadataValue: itemId
								}},
							],
							developerMetadata: {
								metadataValue: itemId
							},
							fields: 'metadataValue'
						}}
					)),
					// delete rows
					...rowGroups.map((group) => (
						{deleteDimension: {
							range: {
								sheetId: 0,
								startIndex: group.startIndex,
								endIndex: group.endIndex,
								dimension: 'ROWS'
							}
						}}
					))
				]
			}
		});
		// transform result
		return {
			indexes: rowIndexes.map((index) => (index - PLAYLIST_ITEMS_START_OFFSET))
		};
	}

	async reorderPlaylistItems(playlistURI, index, count, insertBefore) {
		if(!Number.isInteger(index) || index < 0) {
			throw new Error("index must be a positive integer");
		}
		if(!Number.isInteger(count) || count <= 0) {
			throw new Error("count must be a positive non-zero integer");
		}
		if(!Number.isInteger(insertBefore) || insertBefore < 0) {
			throw new Error("insertBefore must be a positive integer");
		}
		// parse uri
		const uriParts = this._parsePlaylistURI(playlistURI);
		// reorder items
		await this._sheets.spreadsheets.batchUpdate({
			spreadsheetId: uriParts.fileId,
			requestBody: {
				requests: [
					{moveDimension: {
						source: {
							sheetId: 0,
							dimension: 'ROWS',
							startIndex: PLAYLIST_ITEMS_START_OFFSET + index,
							endIndex: PLAYLIST_ITEMS_START_OFFSET + index + count
						},
						destinationIndex: insertBefore
					}}
				]
			}
		});
	}
}


module.exports = GoogleDriveStorageProvider;

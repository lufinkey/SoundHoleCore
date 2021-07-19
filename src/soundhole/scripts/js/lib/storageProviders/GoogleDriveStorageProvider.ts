
import {
	google,
	drive_v3,
	sheets_v4,
	Auth } from 'googleapis';
import { v1 as uuidv1 } from 'uuid';
import {
	GoogleSheetsDB,
	GoogleSheetsDB$InstantiateResult,
	GSDBCellValue,
	GSDBColumnInfo,
	GSDBInfo,
	GSDBRow,
	GSDBTableData,
	GSDBTableSheetInfo } from 'google-sheets-db';
import StorageProvider, {
	CreatePlaylistOptions,
	GetPlaylistItemsOptions,
	GetPlaylistOptions,
	GetUserPlaylistsOptions,
	Image,
	Playlist,
	PlaylistItem,
	PlaylistItemPage,
	PlaylistPage,
	PlaylistPrivacyId,
	Track,
	User } from './StorageProvider';

const PLAYLISTS_FOLDER_NAME = "Playlists";
const PLAYLIST_VERSIONS = [ '1.0' ];
const PLAYLIST_LATEST_VERSION = '1.0';
const PLAYLIST_PRIVACIES = ['public','unlisted','private'];

const PLAYLIST_ITEM_ONLY_COLUMNS = [
	'addedAt',
	'addedBy',
	'addedById'
];


type GoogleDriveStorageProviderOptions = {
	appKey: string
	baseFolderName: string
	clientId: string
	clientSecret: string
	redirectURL: string
	apiKey?: string | null
	session: Auth.Credentials
	mediaItemBuilder?: MediaItemBuilder | null
}

type MediaItemBuilder = {
	parseStorageProviderURI: (uri: string) => URIParts
	createStorageProviderURI: (storageProvider: string, type: string, id: string) => string
	readonly name: string
}

type GoogleDriveIdentity = drive_v3.Schema$About & {
	uri: string
	baseFolderId: string
}

type PlaylistSheetProps = {
	version: string
	columns: string[]
	itemCount: number
	itemsStartRowIndex: number
}

type URIParts = {
	storageProvider: string
	type: string
	id: string
}

type PlaylistIDParts = {
	fileId: string
	baseFolderId?: string | null
}

type PlaylistURIParts = {
	storageProvider: string
	type: string
	fileId: string
	baseFolderId?: string | null
}

type UserIDParts = {
	email?: string
	permissionId?: string
	baseFolderId?: string | null
}

type UserURIParts = {
	storageProvider: string
	type: string
	email?: string
	permissionId?: string
	baseFolderId?: string | null
}

type GoogleDriveRedirectParams = {
	error?: string
	code?: string
}

type LoginWithRedirectParamsOptions = {
	clientId?: string
	codeVerifier: string
}

export default class GoogleDriveStorageProvider implements StorageProvider {
	_options: GoogleDriveStorageProviderOptions
	_baseFolderId: string | null
	_baseFolder: drive_v3.Schema$File | null
	_playlistsFolderId: string | null
	_currentDriveInfo: drive_v3.Schema$About | null
	_auth: Auth.OAuth2Client
	_drive: drive_v3.Drive
	_sheets: sheets_v4.Sheets

	constructor(options: GoogleDriveStorageProviderOptions) {
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
		this._baseFolder = null;
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


	get name(): string {
		return 'googledrive';
	}

	get displayName(): string {
		return "Google Drive";
	}

	get auth(): Auth.OAuth2Client {
		return this._auth;
	}

	get session(): Auth.Credentials | null {
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



	async loginWithRedirectParams(params: GoogleDriveRedirectParams, options: LoginWithRedirectParamsOptions): Promise<void> {
		if(params.error) {
			throw new Error(`Error logging in from redirect: ${params.error}`);
		}
		if(!params.code) {
			throw new Error("Missing expected parameters in redirect URL");
		}
		const { tokens } = await this._auth.getToken({
			code: params.code,
			codeVerifier: options.codeVerifier,
			client_id: options.clientId ?? this._options.clientId
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

	
	
	async _getCurrentDriveInfo(): Promise<drive_v3.Schema$About> {
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

	async getCurrentUser(): Promise<GoogleDriveIdentity | null> {
		if(this.session == null) {
			return null;
		}
		// ensure base folder data is created
		await this._prepareForRequest();
		// get "about" data
		const about = (await this._drive.about.get({
			fields: "*"
		})).data;
		if(!about.user) {
			return null;
		}
		if(!this._baseFolderId) {
			throw new Error("Missing base folder ID");
		}
		// update drive info
		this._currentDriveInfo = about;
		// create identity
		return {
			...about.user,
			uri: this._createUserURIFromUser(about.user, this._baseFolderId),
			baseFolderId: this._baseFolderId
		};
	}



	get _baseFolderPropKey(): string {
		return `${this._options.appKey}_base_folder`;
	}
	get _playlistPropKey(): string {
		return `${this._options.appKey}_playlist`;
	}
	get _imageDataPropKey(): string {
		return `${this._options.appKey}_img_data`;
	}
	get _imageMimeTypePropKey(): string {
		return `${this._options.appKey}_img_mimetype`;
	}



	get canStorePlaylists(): boolean {
		return true;
	}

	async _prepareForRequest() {
		await this._prepareFolders();
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
		let baseFolder = await (async () => {
			const { files } = (await this._drive.files.list({
				q: `mimeType = 'application/vnd.google-apps.folder' and appProperties has { key='${baseFolderPropKey}' and value='true' } and trashed = false`,
				fields: "*",
				spaces: 'drive',
				pageSize: 1
			})).data;
			if(files == null) {
				throw new Error("Unable to find files property in response when searching for base folder");
			}
			return files[0];
		})();
		if(baseFolder != null) {
			if(baseFolder.id == null) {
				throw new Error("Could not get \"id\" property of base folder");
			}
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
		if(baseFolder.id == null) {
			throw new Error("Could not get \"id\" property of newly created base folder");
		}
		this._baseFolderId = baseFolder.id;
		this._baseFolder = baseFolder;
	}

	async _preparePlaylistsFolder() {
		await this._prepareBaseFolder();
		// if we have a playlists folder ID, we can stop
		if(this._playlistsFolderId != null) {
			return;
		}
		// ensure we have the base folder
		const baseFolderId = this._baseFolderId;
		if(baseFolderId == null) {
			throw new Error("Missing base folder while creating playlists folder");
		}
		// check for existing playlists folder
		const folderName = PLAYLISTS_FOLDER_NAME;
		let playlistsFolder = await (async () => {
			const { files } = (await this._drive.files.list({
				q: `mimeType = 'application/vnd.google-apps.folder' and name = '${folderName}' and '${baseFolderId}' in parents and trashed = false`,
				fields: "nextPageToken, files(id, name)",
				spaces: 'drive',
				pageSize: 1
			})).data;
			if(files == null) {
				throw new Error("Unable to find files property in response when searching for playlists folder");
			}
			return files[0];
		})();
		if(playlistsFolder != null) {
			if(playlistsFolder.id == null) {
				throw new Error("Could not get \"id\" property of playlists folder");
			}
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
		if(playlistsFolder.id == null) {
			throw new Error("Could not get \"id\" property of newly created playlists folder");
		}
		this._playlistsFolderId = playlistsFolder.id;
	}



	_parseURI(uri: string): URIParts {
		if(!this._options.mediaItemBuilder || !this._options.mediaItemBuilder.parseStorageProviderURI) {
			const colonIndex1 = uri.indexOf(':');
			if(colonIndex1 === -1) {
				throw new Error("invalid URI "+uri);
			}
			const provider = uri.substring(0, colonIndex1);
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

	_createURI(type: string, id: string): string {
		if(!this._options.mediaItemBuilder || !this._options.mediaItemBuilder.createStorageProviderURI) {
			return `${this.name}:${type}:${id}`;
		}
		return this._options.mediaItemBuilder.createStorageProviderURI(this.name,type,id);
	}



	_parsePlaylistID(playlistId: string): PlaylistIDParts {
		const index = playlistId.indexOf('/');
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

	_createPlaylistID(fileId: string, baseFolderId: string | null): string {
		if(!fileId) {
			throw new Error("missing fileId for _createPlaylistID");
		} else if(!baseFolderId) {
			return fileId;
		}
		return `${baseFolderId}/${fileId}`;
	}



	_parsePlaylistURI(playlistURI: string): PlaylistURIParts {
		const uriParts = this._parseURI(playlistURI);
		const idParts = this._parsePlaylistID(uriParts.id);
		return {
			storageProvider: uriParts.storageProvider,
			type: uriParts.type,
			fileId: idParts.fileId,
			baseFolderId: idParts.baseFolderId
		};
	}

	_createPlaylistURI(fileId: string, baseFolderId: string | null): string {
		return this._createURI('playlist', this._createPlaylistID(fileId, baseFolderId));
	}

	

	_parseUserID(userId: string): UserIDParts {
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

	_createUserID(identifier: string, baseFolderId: string | null): string {
		if(!identifier) {
			throw new Error("missing identifier for _createUserID");
		} else if(!baseFolderId) {
			return identifier;
		}
		return `${baseFolderId}/${identifier}`;
	}

	_createUserIDFromUser(user: drive_v3.Schema$User, baseFolderId: string | null): string {
		const identifier = user.emailAddress ?? user.permissionId;
		if(identifier == null) {
			throw new Error("Cannot create ID for user without permissionId or emailAddress");
		}
		return this._createUserID(identifier, baseFolderId);
	}



	_parseUserURI(userURI: string): UserURIParts {
		const uriParts = this._parseURI(userURI);
		const idParts = this._parseUserID(uriParts.id);
		return {
			storageProvider: uriParts.storageProvider,
			type: uriParts.type,
			email: idParts.email,
			permissionId: idParts.permissionId,
			baseFolderId: idParts.baseFolderId
		};
	}

	_createUserURI(identifier: string, baseFolderId: string | null): string {
		return this._createURI('user', this._createUserID(identifier, baseFolderId));
	}

	_createUserURIFromUser(user: drive_v3.Schema$User, baseFolderId: string | null): string {
		return this._createURI('user', this._createUserIDFromUser(user, baseFolderId));
	}



	_createUserObject(owner: drive_v3.Schema$User, baseFolderId: string | null): User {
		const mediaItemBuilder = this._options.mediaItemBuilder;
		const displayName = owner.displayName ?? owner.emailAddress ?? owner.permissionId;
		if(displayName == null) {
			throw new Error("Could not get displayable name for user");
		}
		return {
			partial: false,
			type: 'user',
			provider: (mediaItemBuilder != null) ? mediaItemBuilder.name : this.name,
			uri: this._createUserURIFromUser(owner, baseFolderId),
			name: displayName,
			images: (owner.photoLink != null) ? [
				{
					url: owner.photoLink,
					size: 'MEDIUM'
				}
			] : []
		};
	}



	_createPlaylistObject(file: drive_v3.Schema$File, userPermissionId: string | null, userFolderId: string | null): Playlist {
		// validate appProperties
		const playlistPropKey = this._playlistPropKey;
		if(file.id == null) {
			throw new Error("Missing id in playlist file object");
		}
		if(file.name == null) {
			throw new Error("Missing name in playlist file object");
		}
		if(file.modifiedTime == null) {
			throw new Error("Missing modifiedTime in playlist file object");
		}
		if(file.owners == null) {
			throw new Error("Missing owners in playlist file object");
		}
		if(file.permissions == null) {
			throw new Error("Missing permissions in playlist file object");
		}
		const owner = file.owners[0];
		if(owner == null) {
			throw new Error("Missing owner in playlist file object");
		}
		if((file.appProperties ?? {})[playlistPropKey] != 'true' && (file.properties ?? {})[playlistPropKey] != 'true') {
			throw new Error("file is not a SoundHole playlist");
		}
		// parse/validate owner and base folder ID
		let baseFolderId: string | null = null;
		if(userPermissionId != null && userFolderId != null && (owner.permissionId === userPermissionId || owner.emailAddress === userPermissionId)) {
			baseFolderId = userFolderId;
		} else if(file.ownedByMe && this._baseFolderId) {
			baseFolderId = this._baseFolderId;
		}
		// TODO parse images
		return {
			partial: false,
			type: 'playlist',
			uri: this._createPlaylistURI(file.id, baseFolderId),
			name: file.name,
			description: file.description ?? "",
			versionId: file.modifiedTime,
			privacy: this._determinePrivacy(file.permissions),
			owner: this._createUserObject(owner, baseFolderId)
		};
	}



	_validatePrivacy(privacy: string) {
		if(privacy && PLAYLIST_PRIVACIES.indexOf(privacy) === -1) {
			throw new Error(`invalid privacy value "${privacy}" for google drive storage provider`);
		}
	}

	_determinePrivacy(permissions: drive_v3.Schema$Permission[]): PlaylistPrivacyId {
		let privacy: PlaylistPrivacyId = 'private';
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



	get _playlistVersionKey(): string {
		return `${this._options.appKey}_playlist_version`;
	}

	get _playlistItemIdKey(): string {
		return `${this._options.appKey}_playlist_item_id`;
	}

	get _playlistItemsTableName(): string {
		return 'items';
	}

	_playlistDBInfo(): GSDBInfo {
		return {
			tables: [
				{ // items table
					name: this._playlistItemsTableName,
					columnInfo: [
						{
							name: 'uri',
							type: 'rawstring',
							nonnull: true
						}, {
							name: 'provider',
							type: 'rawstring',
							nonnull: true
						}, {
							name: 'name',
							type: 'rawstring',
							nonnull: true
						}, {
							name: 'albumName',
							type: 'string'
						}, {
							name: 'albumURI',
							type: 'string'
						}, {
							name: 'artists',
							type: 'any'
						}, {
							name: 'images',
							type: 'any'
						}, {
							name: 'duration',
							type: 'float'
						}, {
							name: 'addedAt',
							type: 'timestamp',
							nonnull: true
						}, {
							name: 'addedById',
							type: 'rawstring',
							nonnull: true
						}, {
							name: 'addedBy',
							type: 'any',
							nonnull: true
						}
					],
					rowMetadataInfo: [
						{
							name: this._playlistItemIdKey,
							type: 'rawstring'
						}
					],
					metadata: {}
				},
			],
			metadata: {
				[this._playlistVersionKey]: PLAYLIST_LATEST_VERSION
			}
		};
	}



	async createPlaylist(name: string, options: CreatePlaylistOptions = {}): Promise<Playlist> {
		await this._prepareForRequest();
		// validate input
		if(options.privacy != null) {
			this._validatePrivacy(options.privacy);
		}
		// prepare folder
		await this._preparePlaylistsFolder();
		const baseFolderId = this._baseFolderId;
		if(baseFolderId == null) {
			throw new Error("missing base folder while creating playlist");
		}
		// get drive user
		const driveUser = (await this._getCurrentDriveInfo()).user;
		if(driveUser == null) {
			throw new Error("Failed to get current drive user");
		}
		const driveUserIdentifier = driveUser.permissionId ?? driveUser.emailAddress;
		if(driveUserIdentifier == null) {
			throw new Error("Could not get drive user identifiers");
		}
		// ensure playlists folder ID
		const playlistsFolderId = this._playlistsFolderId;
		if(playlistsFolderId == null) {
			throw new Error("missing playlists folder ID");
		}
		// prepare appProperties
		const appProperties: {[key: string]: string} = {};
		appProperties[this._playlistPropKey] = 'true';
		if(options.image) {
			appProperties[this._imageDataPropKey] = options.image.data;
			appProperties[this._imageMimeTypePropKey] = options.image.mimeType;
		}
		// create playlist db
		const db = new GoogleSheetsDB({
			drive: this._drive,
			sheets: this._sheets
		});
		// instantiate db
		let instResult: GoogleSheetsDB$InstantiateResult;
		try {
			instResult = await db.instantiate({
				name: name,
				file: {
					description: options.description,
					parents: [playlistsFolderId],
					appProperties: appProperties
				},
				privacy: options.privacy ?? 'private',
				db: this._playlistDBInfo()
			});
		} catch(error) {
			if(db.fileId != null) {
				// try to destroy the db if we can
				db.destroy().catch((error) => {
					console.error(error);
				});
			}
			throw error;
		}
		const { file, privacy } = instResult;
		if(file.id == null) {
			throw new Error("Missing \"id\" property of newly created playlist file");
		}
		// transform result
		const playlist = this._createPlaylistObject(file, driveUserIdentifier, baseFolderId);
		playlist.privacy = privacy;
		playlist.itemCount = 0;
		playlist.items = [];
		return playlist;
	}


	async getPlaylist(playlistURI: string, options: GetPlaylistOptions = {}) {
		await this._prepareForRequest();
		// validate options
		const itemsStartIndex = options.itemsStartIndex ?? 0;
		const itemsLimit = options.itemsLimit ?? 100;
		if(itemsStartIndex != null && (!Number.isInteger(itemsStartIndex) || itemsStartIndex < 0)) {
			throw new Error("options.itemsStartIndex must be a positive integer");
		}
		if(itemsLimit != null && (!Number.isInteger(itemsLimit) || itemsLimit < 0)) {
			throw new Error("options.itemsLimit must be a positive integer");
		}
		// parse uri
		const uriParts = this._parsePlaylistURI(playlistURI);
		// start file promise
		const filePromise = this._drive.files.get({
			fileId: uriParts.fileId,
			fields: "*"
		});
		// get playlist items
		const itemsPage = await this.getPlaylistItems(playlistURI, { offset: itemsStartIndex, limit: itemsLimit });
		// get file
		const file = (await filePromise).data;
		if(!file.owners) {
			throw new Error("Could not get file owner");
		}
		// assume file owner is same as folder owner
		const fileOwner = file.owners[0];
		const fileOwnerIdentifier = fileOwner.permissionId ?? fileOwner.emailAddress;
		if(fileOwnerIdentifier == null) {
			throw new Error("Unable to get identifiers from file owner");
		}
		// transform result
		const playlist = this._createPlaylistObject(file, fileOwnerIdentifier, uriParts.baseFolderId ?? null);
		if(itemsPage) {
			if(itemsPage.total != null) {
				playlist.itemCount = itemsPage.total;
			}
			if(itemsPage.offset == null || itemsPage.offset === 0) {
				playlist.items = itemsPage.items;
			} else {
				const playlistItems: {[key: number]: PlaylistItem} = {};
				let i = itemsPage.offset;
				for(const item of itemsPage.items) {
					playlistItems[i] = item;
					i++;
				}
				playlist.items = playlistItems;
			}
		}
		return playlist;
	}


	async deletePlaylist(playlistURI: string, options: {permanent?: boolean} = {}): Promise<void> {
		await this._prepareForRequest();
		const uriParts = this._parsePlaylistURI(playlistURI);
		if(options.permanent) {
			// delete file
			await this._drive.files.delete({
				fileId: uriParts.fileId
			});
		} else {
			// move file to trash
			await this._drive.files.update({
				fileId: uriParts.fileId,
				requestBody: {
					trashed: true
				}
			});
		}
	}


	async getMyPlaylists(options: GetUserPlaylistsOptions = {}): Promise<PlaylistPage> {
		await this._prepareForRequest();
		// prepare playlists folder
		await this._preparePlaylistsFolder();
		const baseFolder = this._baseFolder;
		if(baseFolder == null) {
			throw new Error("Missing base folder");
		}
		const baseFolderId = baseFolder.id;
		if(baseFolderId == null) {
			throw new Error("Missing \"id\" property of base folder");
		}
		if(baseFolder.owners == null) {
			throw new Error("Missing \"owners\" property of base folder");
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
			pageToken: options.pageToken,
			pageSize: options.pageSize ?? 1000
		})).data;
		// transform result
		return {
			items: (files || []).map((file) => (
				this._createPlaylistObject(file, baseFolderOwner?.permissionId ?? null, baseFolderId)
			)),
			nextPageToken: nextPageToken ?? null
		};
	}


	async getUserPlaylists(userURI: string, options: GetUserPlaylistsOptions = {}): Promise<PlaylistPage> {
		await this._prepareForRequest();
		const uriParts = this._parseUserURI(userURI);
		if(!uriParts.baseFolderId) {
			throw new Error("user URI does not include SoundHole folder ID");
		}
		const userBaseFolderId = uriParts.baseFolderId;
		const userIdentifier = uriParts.permissionId ?? uriParts.email;
		if(userIdentifier == null) {
			throw new Error("Missing user identifier in URI");
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
			const playlistsFolder = await (async () => {
				const { files } = (await this._drive.files.list({
					q: `mimeType = 'application/vnd.google-apps.folder' and name = '${folderName}' and '${userBaseFolderId}' in parents and trashed = false`,
					fields: "*",
					pageSize: 1
				})).data;
				if(files == null) {
					throw new Error("Missing \"files\" property in response when retrieving playlists folder");
				}
				return files[0];
			})();
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
			pageToken: pageToken,
			pageSize: options.pageSize ?? 1000
		})).data;
		if(files == null) {
			throw new Error("Missing \"files\" property in response");
		}
		// transform result
		return {
			items: files.map((file) => (
				this._createPlaylistObject(file, userIdentifier, userBaseFolderId)
			)),
			nextPageToken: (nextPageToken ? `${playlistsFolderId}:${nextPageToken}` : null)
		};
	}



	async isPlaylistEditable(playlistURI: string): Promise<boolean> {
		await this._prepareForRequest();
		const driveInfo = await this._getCurrentDriveInfo();
		const uriParts = this._parsePlaylistURI(playlistURI);
		let done = false;
		let pageToken: string | null | undefined = null;
		const editRoles = ['writer','fileOrganizer','organizer','owner'];
		while(!done) {
			const { permissions, nextPageToken } = (await this._drive.permissions.list({
				fileId: uriParts.fileId,
				pageSize: 100,
				pageToken: pageToken ?? undefined
			})).data as drive_v3.Schema$PermissionList;
			if(permissions == null) {
				throw new Error("Missing \"permissions\" property on playlist permissions list");
			}
			for(const p of permissions) {
				if(p.role == null) {
					continue;
				}
				if(editRoles.indexOf(p.role) === -1) {
					continue;
				}
				if(p.type === 'anyone') {
					return true;
				}
				else if(p.type === 'user' && driveInfo.user
					&& ((p.emailAddress != null && p.emailAddress === driveInfo.user.emailAddress)
						|| (p.id != null && p.id === driveInfo.user.permissionId))) {
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



	_formatDate(date: Date): string {
		// TODO change this to just return ISO string in the future
		const fixedDate = new Date(Math.floor(date.getTime() / 1000) * 1000);
		let dateString = fixedDate.toISOString();
		const suffix = ".000Z";
		if(dateString.endsWith(suffix)) {
			dateString = dateString.substring(0,dateString.length-suffix.length)+'Z';
		}
		return dateString;
	}

	_parsePlaylistItemTableRow(row: GSDBRow, columns: GSDBColumnInfo[], index: number): PlaylistItem {
		if(row.values == null) {
			throw new Error("Missing values property for playlist row");
		}
		if(row.values.length < columns.length) {
			throw new Error("Not enough columns in row");
		}
		const item: any = {};
		const track: any = {};
		for(let i=0; i<columns.length; i++) {
			const column = columns[i];
			const value = row.values[i];
			if(PLAYLIST_ITEM_ONLY_COLUMNS.indexOf(column.name) !== -1) {
				item[column.name] = value;
			} else {
				track[column.name] = value;
			}
		}
		const itemId = row.metadata[this._playlistItemIdKey];
		if(itemId == null) {
			throw new Error(`Missing metadata property ${this._playlistItemIdKey} in row ${index}`);
		}
		item.uniqueId = itemId;
		if(track.type == null) {
			track.type = 'track';
		}
		item.track = track;
		if(item.addedAt != null) {
			// format timestamp as date string
			item.addedAt = this._formatDate(new Date(item.addedAt));
		}
		return item;
	}

	_parsePlaylistItemsTableData(tableData: GSDBTableSheetInfo & GSDBTableData): PlaylistItemPage {
		const items: PlaylistItem[] = [];
		for(const row of tableData.rows) {
			const index = tableData.rowsOffset + items.length;
			items.push(this._parsePlaylistItemTableRow(row, tableData.columnInfo, index));
		}
		return {
			offset: tableData.rowsOffset,
			total: tableData.rowCount,
			items
		};
	}


	async getPlaylistItems(playlistURI: string, { offset, limit }: GetPlaylistItemsOptions): Promise<PlaylistItemPage> {
		await this._prepareForRequest();
		if(!Number.isInteger(offset) || offset < 0) {
			throw new Error("offset must be a positive integer");
		} else if(!Number.isInteger(limit) || limit <= 0) {
			throw new Error("limit must be a positive non-zero integer");
		}
		// parse uri
		const uriParts = this._parsePlaylistURI(playlistURI);
		// get db data
		const db = new GoogleSheetsDB({
			fileId: uriParts.fileId,
			drive: this._drive,
			sheets: this._sheets
		});
		if(offset === 0) {
			// get table info and item rows together
			const tableData = await db.getTableData(this._playlistItemsTableName, {
				offset,
				limit
			});
			return this._parsePlaylistItemsTableData(tableData);
		} else {
			// get table info
			const tableInfo = await db.getTableInfo(this._playlistItemsTableName);
			// if we're out of range, return an empty page
			if(offset >= tableInfo.rowCount) {
				return {
					total: tableInfo.rowCount,
					offset: tableInfo.rowCount,
					items: []
				};
			}
			// get items rows and spreadsheet properties
			const tableData = await db.getTableData(this._playlistItemsTableName, {
				offset,
				limit
			});
			return this._parsePlaylistItemsTableData(tableData);
		}
	}



	async _insertPlaylistItems(db: GoogleSheetsDB, index: number, tracks: Track[], tableInfo: GSDBTableSheetInfo, driveInfo: drive_v3.Schema$About): Promise<PlaylistItemPage> {
		const rowItemIdKey = this._playlistItemIdKey;
		if(!driveInfo.user) {
			throw new Error("Missing user in drive info");
		}
		// build items data
		const addedAt = (new Date()).getTime();
		const addedBy = this._createUserObject(driveInfo.user, this._baseFolderId);
		const addedById = this._createUserIDFromUser(driveInfo.user, this._baseFolderId);
		const items: PlaylistItem[] = tracks.map((track): PlaylistItem & {addedById: string} => ({
			uniqueId: uuidv1(),
			addedAt,
			addedBy,
			addedById,
			track
		}));
		// insert tracks into playlist
		await db.insertTableRows(tableInfo, index, items.map((item): GSDBRow => {
			const values: GSDBCellValue[] = [];
			for(const columnInfo of tableInfo.columnInfo) {
				let value: GSDBCellValue;
				if(PLAYLIST_ITEM_ONLY_COLUMNS.includes(columnInfo.name)) {
					value = (item as any)[columnInfo.name];
				} else {
					value = (item.track as any)[columnInfo.name];
				}
				values.push(value);
			}
			const metadata: {[key: string]: GSDBCellValue} = {
				[rowItemIdKey]: item.uniqueId
			};
			return {
				values,
				metadata
			};
		}));
		return {
			offset: index,
			total: tableInfo.rowCount,
			items
		};
	}

	async insertPlaylistItems(playlistURI: string, index: number, tracks: Track[]): Promise<PlaylistItemPage> {
		if(!Number.isInteger(index) || index < 0) {
			throw new Error("index must be a positive integer");
		}
		await this._prepareForRequest();
		// parse uri
		const uriParts = this._parsePlaylistURI(playlistURI);
		// get drive info
		const driveInfo = await this._getCurrentDriveInfo();
		// get spreadsheet info
		const db = new GoogleSheetsDB({
			fileId: uriParts.fileId,
			drive: this._drive,
			sheets: this._sheets
		});
		const tableInfo = await db.getTableInfo(this._playlistItemsTableName);
		// ensure index is in range
		if(index > tableInfo.rowCount) {
			throw new Error(`index ${index} is out of bounds in playlist with URI ${playlistURI}`);
		}
		// insert new rows
		return await this._insertPlaylistItems(db, index, tracks, tableInfo, driveInfo);
	}

	async appendPlaylistItems(playlistURI: string, tracks: Track[]): Promise<PlaylistItemPage> {
		await this._prepareForRequest();
		// parse uri
		const uriParts = this._parsePlaylistURI(playlistURI);
		// get drive info
		const driveInfo = await this._getCurrentDriveInfo();
		// get spreadsheet info
		const db = new GoogleSheetsDB({
			fileId: uriParts.fileId,
			drive: this._drive,
			sheets: this._sheets
		});
		const tableInfo = await db.getTableInfo(this._playlistItemsTableName);
		// append new rows
		return await this._insertPlaylistItems(db, tableInfo.rowCount, tracks, tableInfo, driveInfo);
	}

	async deletePlaylistItems(playlistURI: string, itemIds: string[]): Promise<{indexes: number[]}> {
		if(itemIds.length === 0) {
			return {
				indexes: []
			};
		}
		await this._prepareForRequest();
		const rowItemIdKey = this._playlistItemIdKey;
		// parse uri
		const uriParts = this._parsePlaylistURI(playlistURI);
		// delete rows
		const db = new GoogleSheetsDB({
			fileId: uriParts.fileId,
			drive: this._drive,
			sheets: this._sheets
		});
		return await db.deleteTableRowsWithMetadata(this._playlistItemsTableName, itemIds.map((itemId) => {
			return {
				key: rowItemIdKey,
				value: itemId,
				match: 'any'
			};
		}));
	}

	async movePlaylistItems(playlistURI: string, index: number, count: number, newIndex: number) {
		if(!Number.isInteger(index) || index < 0) {
			throw new Error("index must be a positive integer");
		}
		if(!Number.isInteger(count) || count <= 0) {
			throw new Error("count must be a positive non-zero integer");
		}
		if(!Number.isInteger(newIndex) || newIndex < 0) {
			throw new Error("insertBefore must be a positive integer");
		}
		await this._prepareForRequest();
		// parse uri
		const uriParts = this._parsePlaylistURI(playlistURI);
		// reorder items
		const db = new GoogleSheetsDB({
			fileId: uriParts.fileId,
			drive: this._drive,
			sheets: this._sheets
		});
		await db.moveTableRows(this._playlistItemsTableName, index, count, newIndex);
	}
}

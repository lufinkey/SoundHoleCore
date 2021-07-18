
import {
	google,
	drive_v3,
	sheets_v4,
	Auth } from 'googleapis';
import { v1 as uuidv1 } from 'uuid';
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
	get _playlistItemIdKey(): string {
		return `${this._options.appKey}_playlist_item_id`;
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



	async createPlaylist(name: string, options: CreatePlaylistOptions = {}): Promise<Playlist> {
		await this._prepareForRequest();
		// validate input
		const privacy = options.privacy ?? 'private';
		this._validatePrivacy(privacy);
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
		if(file.id == null) {
			throw new Error("Missing \"id\" property of newly created playlist file");
		}
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
		const playlist = this._createPlaylistObject(file, driveUserIdentifier, baseFolderId);
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



	_indexToColumn(index: number): string {
		let letter = '', temp;
		while(index >= 0) {
			temp = index % 26;
			letter = String.fromCharCode(temp + 65) + letter;
			index = (index - temp - 1) / 26;
		}
		return letter;
	}
	_a1Notation(x: number, y: number): string {
		return ''+this._indexToColumn(x)+(y+1);
	}
	_a1Range(x1: number, y1: number, x2: number, y2: number): string {
		return this._a1Notation(x1,y1)+':'+this._a1Notation(x2,y2)
	}
	_playlistHeaderA1Range(): string {
		return this._a1Range(0,0, (PLAYLIST_COLUMNS.length+PLAYLIST_EXTRA_COLUMN_COUNT), 1);
	}
	_playlistItemsA1Range(startIndex: number, itemCount: number): string {
		if(!Number.isInteger(startIndex) || startIndex < 0) {
			throw new Error("startIndex must be a positive integer");
		} else if(!Number.isInteger(itemCount) || itemCount <= 0) {
			throw new Error("itemCount must be a positive non-zero integer");
		}
		return this._a1Range(0,(PLAYLIST_ITEMS_START_OFFSET+startIndex), (PLAYLIST_COLUMNS.length+PLAYLIST_EXTRA_COLUMN_COUNT), (PLAYLIST_ITEMS_START_OFFSET+itemCount));
	}
	_playlistHeaderAndItemsA1Range(itemCount: number): string {
		return this._a1Range(0,0, (PLAYLIST_COLUMNS.length+PLAYLIST_EXTRA_COLUMN_COUNT), (1+itemCount));
	}
	_playlistColumnsA1Range(): string {
		return this._a1Range(0,1, PLAYLIST_COLUMNS.length,1);
	}

	_parseUserEnteredValue(val: sheets_v4.Schema$ExtendedValue): any {
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

	async _preparePlaylistSheet(fileId: string) {
		// get spreadsheet header
		const spreadsheet = (await this._sheets.spreadsheets.get({
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
		if(gridProps == null || gridProps.columnCount == null || gridProps.rowCount == null) {
			throw new Error("Missing spreadsheet gridProperties");
		}
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

	_parsePlaylistSheetProperties(spreadsheet: sheets_v4.Schema$Spreadsheet, rangeIndex: number): PlaylistSheetProps {
		// get sheet data
		if(!spreadsheet || !spreadsheet.sheets || !spreadsheet.sheets[0]) {
			throw new Error(`Missing spreadsheet sheets`);
		}
		const sheet = spreadsheet.sheets[0];
		if(!sheet.data || !sheet.data[rangeIndex] || !sheet.data[rangeIndex].rowData) {
			throw new Error(`Missing spreadsheet data`);
		}
		const gridProps = sheet.properties?.gridProperties;
		if(gridProps == null || gridProps.rowCount == null || gridProps.columnCount == null) {
			throw new Error("Missing gridProps in playlist sheet");
		}
		// calculate item count
		const itemsStartOffset = PLAYLIST_ITEMS_START_OFFSET;
		const itemCount = gridProps.rowCount - itemsStartOffset;
		// parse top row properties
		const data = sheet.data[rangeIndex];
		if(!data) {
			throw new Error(`Missing range data at index ${rangeIndex}`);
		}
		if(data.rowData == null) {
			throw new Error(`Missing row data for range index ${rangeIndex}`);
		}
		if(!data.rowData[0]) {
			throw new Error("Missing top row of playlist sheet");
		}
		const topRowDataValues = data.rowData[0].values;
		if(!topRowDataValues || !topRowDataValues[0]) {
			throw new Error("Missing version cell for playlist sheet");
		}
		if(!topRowDataValues[0].userEnteredValue) {
			throw new Error("Missing userEnteredValue for version cell in playlist sheet");
		}
		const version = this._parseUserEnteredValue(topRowDataValues[0].userEnteredValue);
		// parse columns
		if(!data.rowData[1] || !data.rowData[1].values) {
			throw new Error("Missing columns row values in playlist sheet");
		}
		let columns = [];
		for(const cell of data.rowData[1].values) {
			if(!cell.userEnteredValue) {
				throw new Error(`Missing userEnteredValue for column cell ${columns.length} in playlist sheet`);
			}
			const cellValue = this._parseUserEnteredValue(cell.userEnteredValue);
			if(cellValue != null && cellValue !== '') {
				columns.push(`${cellValue}`);
			} else {
				break;
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
		return {
			version,
			columns,
			itemCount,
			itemsStartRowIndex: itemsStartOffset
		};
	}

	_parsePlaylistSheetItems(spreadsheet: sheets_v4.Schema$Spreadsheet, rangeIndex: number, startOffset: number, { columns, itemCount, itemsStartRowIndex }: { columns: string[], itemCount: number, itemsStartRowIndex: number }): PlaylistItemPage {
		// parse out sheet data
		if(!spreadsheet || !spreadsheet.sheets || !spreadsheet.sheets[0]) {
			throw new Error(`Missing spreadsheet sheets`);
		}
		const sheet = spreadsheet.sheets[0];
		if(!sheet.data || !sheet.data[rangeIndex] || !sheet.data[rangeIndex].rowData) {
			throw new Error(`Missing spreadsheet data`);
		}
		const data = sheet.data[rangeIndex];
		// ensure row data
		if(!data.rowData) {
			throw new Error("Missing row data for playlist sheet");
		}
		if(!data.rowMetadata) {
			throw new Error("Missing row metadata for playlist sheet");
		}
		// parse tracks
		const startIndex = (data.startRow ?? 0) + startOffset - itemsStartRowIndex;
		const items = [];
		let i = 0;
		let skippedRowCount = 0;
		for(const row of data.rowData) {
			if(skippedRowCount < startOffset) {
				skippedRowCount++;
				i++;
				continue;
			}
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

	_parsePlaylistSheetItemRow(row: sheets_v4.Schema$RowData, rowMetadata: sheets_v4.Schema$DimensionProperties, columns: string[], index: number): PlaylistItem {
		if(row.values == null) {
			throw new Error("Missing values property for playlist row");
		}
		if(row.values.length < columns.length) {
			throw new Error("Not enough columns in row");
		}
		const rowItemIdKey = this._playlistItemIdKey;
		const item: any = {};
		const track: any = {};
		for(let i=0; i<columns.length; i++) {
			const columnName = columns[i];
			const cell = row.values[i];
			if(!cell.userEnteredValue) {
				throw new Error(`Missing userEnteredValue for ${columnName} cell`);
			}
			const value = JSON.parse(this._parseUserEnteredValue(cell.userEnteredValue));
			if(PLAYLIST_ITEM_ONLY_COLUMNS.indexOf(columnName) !== -1) {
				item[columnName] = value;
			} else {
				track[columnName] = value;
			}
			const itemIdMetadata = rowMetadata.developerMetadata?.find((metadata) => {
				return metadata.metadataKey === rowItemIdKey
			});
			if(!itemIdMetadata) {
				throw new Error(`Missing developer metadata '${rowItemIdKey}' for track at index ${index}`);
			}
			item.uniqueId = itemIdMetadata.metadataValue;
		}
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


	async getPlaylistItems(playlistURI: string, { offset, limit }: GetPlaylistItemsOptions): Promise<PlaylistItemPage> {
		await this._prepareForRequest();
		if(!Number.isInteger(offset) || offset < 0) {
			throw new Error("offset must be a positive integer");
		} else if(!Number.isInteger(limit) || limit <= 0) {
			throw new Error("limit must be a positive non-zero integer");
		}
		// parse uri
		const uriParts = this._parsePlaylistURI(playlistURI);
		if(offset === 0) {
			// get spreadsheet properties and item rows together
			const spreadsheet = (await this._sheets.spreadsheets.get({
				spreadsheetId: uriParts.fileId,
				includeGridData: true,
				ranges: [
					this._playlistHeaderAndItemsA1Range(limit)
				]
			})).data;
			const sheetProps = this._parsePlaylistSheetProperties(spreadsheet, 0);
			const itemsPage = this._parsePlaylistSheetItems(spreadsheet, 0, 2, sheetProps);
			return itemsPage;
		} else {
			// get spreadsheet properties
			let spreadsheet = (await this._sheets.spreadsheets.get({
				spreadsheetId: uriParts.fileId,
				includeGridData: true,
				ranges: [
					this._playlistHeaderA1Range()
				]
			})).data;
			let sheetProps = this._parsePlaylistSheetProperties(spreadsheet, 0);
			// if we're out of range, return an empty page
			if(offset >= sheetProps.itemCount) {
				return {
					total: sheetProps.itemCount,
					offset: sheetProps.itemCount,
					items: []
				}
			}
			// get items rows and spreadsheet properties
			spreadsheet = (await this._sheets.spreadsheets.get({
				spreadsheetId: uriParts.fileId,
				includeGridData: true,
				ranges: [
					this._playlistHeaderA1Range(),
					this._playlistItemsA1Range(offset, limit)
				]
			})).data;
			sheetProps = this._parsePlaylistSheetProperties(spreadsheet, 0);
			const itemsPage = this._parsePlaylistSheetItems(spreadsheet, 1, 0, sheetProps);
			return itemsPage;
		}
	}



	async _insertPlaylistItems(playlistURI: string, index: number, tracks: Track[], sheetProps: PlaylistSheetProps, driveInfo: drive_v3.Schema$About): Promise<PlaylistItemPage> {
		const rowItemIdKey = this._playlistItemIdKey;
		if(!driveInfo.user) {
			throw new Error("Missing user in drive info");
		}
		// parse uri
		const uriParts = this._parsePlaylistURI(playlistURI);
		// build items data
		const addedAt = (new Date()).getTime();
		const addedBy = this._createUserObject(driveInfo.user, this._baseFolderId);
		const addedById = `/${this._createUserIDFromUser(driveInfo.user, this._baseFolderId)}/`;
		const items: PlaylistItem[] = tracks.map((track) => (
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
								if(PLAYLIST_ITEM_ONLY_COLUMNS.indexOf(columnName) !== -1) {
									trackProp = (item as any)[columnName];
								} else {
									trackProp = (track as any)[columnName];
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
		const spreadsheet = (await this._sheets.spreadsheets.get({
			spreadsheetId: uriParts.fileId,
			includeGridData: true,
			ranges: [
				this._playlistHeaderA1Range()
			]
		})).data;
		const sheetProps = this._parsePlaylistSheetProperties(spreadsheet, 0);
		// ensure index is in range
		if(index > sheetProps.itemCount) {
			throw new Error(`index ${index} is out of bounds in playlist with URI ${playlistURI}`);
		}
		// insert new rows
		return await this._insertPlaylistItems(playlistURI, index, tracks, sheetProps, driveInfo);
	}

	async appendPlaylistItems(playlistURI: string, tracks: Track[]): Promise<PlaylistItemPage> {
		await this._prepareForRequest();
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

	async deletePlaylistItems(playlistURI: string, itemIds: string[]): Promise<void> {
		if(itemIds.length === 0) {
			return;
		}
		await this._prepareForRequest();
		const rowItemIdKey = this._playlistItemIdKey;
		// parse uri
		const uriParts = this._parsePlaylistURI(playlistURI);
		// find matching item ids
		const { matchedDeveloperMetadata } = (await this._sheets.spreadsheets.developerMetadata.search({
			spreadsheetId: uriParts.fileId,
			requestBody: {
				dataFilters: itemIds.map((itemId) => ({
					developerMetadataLookup: {
						locationType: 'ROW',
						metadataKey: rowItemIdKey,
						metadataValue: itemId
					}
				}))
			}
		})).data;
		// get row indexes
		const rows = itemIds.map((itemId) => {
			const metadata = (matchedDeveloperMetadata ?? []).find((metadata) => {
				const devMeta = metadata.developerMetadata;
				return (devMeta?.metadataKey === rowItemIdKey && devMeta.metadataValue == itemId);
			});
			if(!metadata) {
				throw new Error(`Could not find playlist item with id ${itemId}`);
			}
			const rowIndex = metadata.developerMetadata?.location?.dimensionRange?.startIndex;
			if(rowIndex == null) {
				throw new Error(`Missing rowIndex property of developer metadata`);
			}
			return {
				itemId,
				rowIndex
			};
		});
		rows.sort((a, b) => (a.rowIndex - b.rowIndex));
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
				expectedNextIndex = rowIndex + 1;
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
									metadataLocation: {
										locationType: 'ROW',
										dimensionRange: {
											sheetId: 0,
											startIndex: rowIndex,
											endIndex: rowIndex + 1,
											dimension: 'ROWS'
										}
									},
									locationMatchingStrategy: 'EXACT_LOCATION',
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
		/*return {
			indexes: rows.map((row) => (row.index - PLAYLIST_ITEMS_START_OFFSET))
		};*/
	}

	async reorderPlaylistItems(playlistURI: string, index: number, count: number, insertBefore: number) {
		if(!Number.isInteger(index) || index < 0) {
			throw new Error("index must be a positive integer");
		}
		if(!Number.isInteger(count) || count <= 0) {
			throw new Error("count must be a positive non-zero integer");
		}
		if(!Number.isInteger(insertBefore) || insertBefore < 0) {
			throw new Error("insertBefore must be a positive integer");
		}
		await this._prepareForRequest();
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
						destinationIndex: PLAYLIST_ITEMS_START_OFFSET + insertBefore
					}}
				]
			}
		});
	}
}

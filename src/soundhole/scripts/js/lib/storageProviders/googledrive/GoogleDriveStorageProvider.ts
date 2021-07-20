
import {
	google,
	drive_v3,
	sheets_v4,
	Auth } from 'googleapis';
import { determinePermissionsPrivacy } from 'google-sheets-db';
import { v1 as uuidv1 } from 'uuid';
import StorageProvider, {
	CreatePlaylistOptions,
	createStorageProviderURI,
	GetPlaylistItemsOptions,
	GetPlaylistOptions,
	GetUserPlaylistsOptions,
	MediaItemBuilder,
	parseStorageProviderURI,
	Playlist,
	PlaylistItem,
	PlaylistItemPage,
	PlaylistPage,
	Track,
	URIParts,
	User, 
	validatePlaylistPrivacy} from '../StorageProvider';
import {
	GoogleDriveIdentity,
	GoogleDriveSession,
	PlaylistURIParts,
	UserURIParts,
	PLAYLISTS_FOLDER_NAME,
	parsePlaylistURI,
	createPlaylistURI,
	parseUserURI,
	createUserURI,
	createUserURIFromUser,
	createUserIDFromUser } from './types';
import GoogleDrivePlaylist, { InsertingPlaylistItem } from './GoogleDrivePlaylist';
import GoogleDriveLibraryDB from './GoogleDriveLibraryDB';


type GoogleDriveStorageProviderOptions = {
	appKey: string
	baseFolderName: string
	clientId: string
	clientSecret: string
	redirectURL: string
	apiKey?: string | null
	session?: GoogleDriveSession | null
	mediaItemBuilder?: MediaItemBuilder | null
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
	private _options: GoogleDriveStorageProviderOptions
	private _baseFolderId: string | null
	private _baseFolder: drive_v3.Schema$File | null
	private _playlistsFolderId: string | null
	private _currentDriveInfo: drive_v3.Schema$About | null
	private _libraryDB: GoogleDriveLibraryDB | null
	private _auth: Auth.OAuth2Client
	private _drive: drive_v3.Drive
	private _sheets: sheets_v4.Sheets

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
		this._libraryDB = null;
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

	get session(): GoogleDriveSession | null {
		const credentials = this._auth.credentials;
		if(!credentials) {
			return null;
		}
		const accessToken = credentials.access_token;
		const expireDate = credentials.expiry_date;
		const tokenType = credentials.token_type;
		if(!accessToken || !expireDate || !tokenType) {
			return null;
		}
		return {
			...credentials,
			access_token: accessToken,
			expiry_date: expireDate,
			token_type: tokenType
		};
	}

	get isLoggedIn(): boolean {
		return (this.session != null);
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
		this._baseFolderId = null;
		this._baseFolder = null;
		this._playlistsFolderId = null;
		this._libraryDB = null;
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



	async _prepareForRequest() {
		await this._prepareFolders();
		await this._prepareLibraryDB();
	}

	async _prepareFolders() {
		await this._prepareBaseFolder();
		await this._preparePlaylistsFolder();
	}

	get _baseFolderPropKey(): string {
		return `${this._options.appKey}_base_folder`;
	}

	async _prepareBaseFolder() {
		if(!this.isLoggedIn) {
			return;
		}
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
		if(!this.isLoggedIn) {
			return;
		}
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
		let playlistsFolder = await (async () => {
			const { files } = (await this._drive.files.list({
				q: `mimeType = 'application/vnd.google-apps.folder' and name = '${PLAYLISTS_FOLDER_NAME}' and '${baseFolderId}' in parents and trashed = false`,
				fields: "nextPageToken, files(id, name)",
				spaces: 'drive',
				pageSize: 1
			})).data;
			if(files == null) {
				throw new Error(`Unable to find files property in response when searching for "${PLAYLISTS_FOLDER_NAME}" folder`);
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
				name: PLAYLISTS_FOLDER_NAME,
				mimeType: 'application/vnd.google-apps.folder',
				parents: [ baseFolderId ],
			}
		})).data;
		if(playlistsFolder.id == null) {
			throw new Error("Could not get \"id\" property of newly created playlists folder");
		}
		this._playlistsFolderId = playlistsFolder.id;
	}

	async _prepareLibraryDB() {
		if(!this.isLoggedIn) {
			return;
		}
		await this._prepareBaseFolder();
		if(this._libraryDB != null && this._libraryDB.isInstantiated) {
			return;
		}
		const baseFolderId = this._baseFolderId;
		if(!baseFolderId) {
			throw new Error("No base folder found to create DB inside of");
		}
		// instantiate DB if needed
		if(this._libraryDB == null) {
			this._libraryDB = await GoogleDriveLibraryDB.prepare({
				appKey: this._options.appKey,
				folderId: baseFolderId,
				drive: this._drive,
				sheets: this._sheets
			});
		} else {
			await this._libraryDB.instantiateIfNeeded({
				folderId: baseFolderId
			});
		}
	}



	_parseURI(uri: string): URIParts {
		const uriParts = parseStorageProviderURI(uri, this._options.mediaItemBuilder ?? null);
		if(uriParts.storageProvider !== this.name) {
			throw new Error(`URI provider ${uriParts.storageProvider} does not match expected provider name ${this.name}`);
		}
		return uriParts;
	}

	_createURI(type: string, id: string): string {
		return createStorageProviderURI({
			storageProvider: this.name,
			type,
			id
		}, this._options.mediaItemBuilder ?? null);
	}



	_parsePlaylistURI(playlistURI: string): PlaylistURIParts {
		const uriParts = parsePlaylistURI(playlistURI, this._options.mediaItemBuilder ?? null);
		if(uriParts.storageProvider !== this.name) {
			throw new Error(`URI provider ${uriParts.storageProvider} does not match expected provider name ${this.name}`);
		}
		return uriParts;
	}

	_createPlaylistURI(fileId: string, baseFolderId: string | null): string {
		return createPlaylistURI({
			storageProvider: this.name,
			type: 'playlist',
			fileId,
			baseFolderId
		}, this._options.mediaItemBuilder ?? null);
	}



	_parseUserURI(userURI: string): UserURIParts {
		const uriParts = parseUserURI(userURI, this._options.mediaItemBuilder ?? null);
		if(uriParts.storageProvider !== this.name) {
			throw new Error(`URI provider ${uriParts.storageProvider} does not match expected provider name ${this.name}`);
		}
		return uriParts;
	}

	_createUserURI({email, permissionId, baseFolderId}: {email?: string, permissionId?: string, baseFolderId: string | null }): string {
		return createUserURI({
			storageProvider: this.name,
			type: 'user',
			email,
			permissionId,
			baseFolderId
		}, this._options.mediaItemBuilder ?? null);
	}

	_createUserURIFromUser(user: drive_v3.Schema$User, baseFolderId: string | null): string {
		return createUserURIFromUser({
			storageProvider: this.name,
			user,
			baseFolderId
		}, this._options.mediaItemBuilder ?? null);
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
			privacy: determinePermissionsPrivacy(file.permissions),
			owner: this._createUserObject(owner, baseFolderId)
		};
	}



	async createPlaylist(name: string, options: CreatePlaylistOptions = {}): Promise<Playlist> {
		await this._prepareForRequest();
		// validate input
		if(options.privacy != null) {
			validatePlaylistPrivacy(options.privacy);
		}
		// prepare folder
		const baseFolderId = this._baseFolderId;
		if(baseFolderId == null) {
			throw new Error("missing base folder ID");
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
		// create playlist
		const gdbPlaylist = await GoogleDrivePlaylist.create({
			appKey: this._options.appKey,
			folderId: playlistsFolderId,
			drive: this._drive,
			sheets: this._sheets,

			name: name,
			...options
		});
		// ensure file and privacy are set
		if(gdbPlaylist.file == null) {
			throw new Error(`missing file property in newly created playlist`);
		}
		if(gdbPlaylist.privacy == null) {
			throw new Error(`missing privacy property in newly created playlist`);
		}
		// transform result
		const playlist = this._createPlaylistObject(gdbPlaylist.file, driveUserIdentifier, baseFolderId);
		playlist.privacy = gdbPlaylist.privacy;
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
		// get playlist data
		const gdbPlaylist = GoogleDrivePlaylist.forFileID(uriParts.fileId, {
			appKey: this._options.appKey,
			drive: this._drive,
			sheets: this._sheets
		});
		// start file promise
		const filePromise = gdbPlaylist.fetchFile();
		// get playlist items
		const itemsPage = await gdbPlaylist.fetchItems({
			offset: itemsStartIndex,
			limit: itemsLimit
		});
		// get file
		const file = await filePromise;
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
		const gdbPlaylist = GoogleDrivePlaylist.forFileID(uriParts.fileId, {
			appKey: this._options.appKey,
			drive: this._drive,
			sheets: this._sheets
		});
		await gdbPlaylist.delete(options);
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
			const playlistsFolder = await (async () => {
				const { files } = (await this._drive.files.list({
					q: `mimeType = 'application/vnd.google-apps.folder' and name = '${PLAYLISTS_FOLDER_NAME}' and '${userBaseFolderId}' in parents and trashed = false`,
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
		if(!driveInfo.user) {
			return false;
		}
		const uriParts = this._parsePlaylistURI(playlistURI);
		const gdbPlaylist = GoogleDrivePlaylist.forFileID(uriParts.fileId, {
			appKey: this._options.appKey,
			drive: this._drive,
			sheets: this._sheets
		});
		return await gdbPlaylist.userCanEdit(driveInfo.user);
	}



	async getPlaylistItems(playlistURI: string, { offset, limit }: GetPlaylistItemsOptions): Promise<PlaylistItemPage> {
		await this._prepareForRequest();
		// parse uri
		const uriParts = this._parsePlaylistURI(playlistURI);
		// get playlist data
		const gdbPlaylist = GoogleDrivePlaylist.forFileID(uriParts.fileId, {
			appKey: this._options.appKey,
			drive: this._drive,
			sheets: this._sheets
		});
		return gdbPlaylist.fetchItems({
			offset,
			limit
		});
	}



	_playlistItemsFromTracks(tracks: Track[], user: drive_v3.Schema$User, baseFolderId: string): InsertingPlaylistItem[] {
		const addedAt = (new Date()).getTime();
		const addedBy = this._createUserObject(user, baseFolderId);
		const addedById = createUserIDFromUser(user, baseFolderId);
		return tracks.map((track): InsertingPlaylistItem => ({
			uniqueId: uuidv1(),
			addedAt,
			addedBy,
			addedById,
			track
		}));
	}

	async insertPlaylistItems(playlistURI: string, index: number, tracks: Track[]): Promise<PlaylistItemPage> {
		if(!Number.isInteger(index) || index < 0) {
			throw new Error("index must be a positive integer");
		}
		await this._prepareForRequest();
		// ensure base folder ID
		const baseFolderId = this._baseFolderId;
		if(!baseFolderId) {
			throw new Error("Missing base folder ID");
		}
		// parse uri
		const uriParts = this._parsePlaylistURI(playlistURI);
		// get drive info
		const driveInfo = await this._getCurrentDriveInfo();
		if(!driveInfo.user) {
			throw new Error("No drive user available");
		}
		// create items
		const playlistItems = this._playlistItemsFromTracks(tracks, driveInfo.user, baseFolderId);
		// insert items on playlist
		const gdbPlaylist = GoogleDrivePlaylist.forFileID(uriParts.fileId, {
			appKey: this._options.appKey,
			drive: this._drive,
			sheets: this._sheets
		});
		return await gdbPlaylist.insertItems(index, playlistItems);
	}

	async appendPlaylistItems(playlistURI: string, tracks: Track[]): Promise<PlaylistItemPage> {
		await this._prepareForRequest();
		// ensure base folder ID
		const baseFolderId = this._baseFolderId;
		if(!baseFolderId) {
			throw new Error("Missing base folder ID");
		}
		// parse uri
		const uriParts = this._parsePlaylistURI(playlistURI);
		// get drive info
		const driveInfo = await this._getCurrentDriveInfo();
		if(!driveInfo.user) {
			throw new Error("No drive user available");
		}
		// create items
		const playlistItems = this._playlistItemsFromTracks(tracks, driveInfo.user, baseFolderId);
		// append items on playlist
		const gdbPlaylist = GoogleDrivePlaylist.forFileID(uriParts.fileId, {
			appKey: this._options.appKey,
			drive: this._drive,
			sheets: this._sheets
		});
		return await gdbPlaylist.appendItems(playlistItems);
	}

	async deletePlaylistItems(playlistURI: string, itemIds: string[]): Promise<{indexes: number[]}> {
		await this._prepareForRequest();
		// parse uri
		const uriParts = this._parsePlaylistURI(playlistURI);
		// delete playlist items
		const gdbPlaylist = GoogleDrivePlaylist.forFileID(uriParts.fileId, {
			appKey: this._options.appKey,
			drive: this._drive,
			sheets: this._sheets
		});
		return await gdbPlaylist.deleteItemsWithIDs(itemIds);
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
		const gdbPlaylist = GoogleDrivePlaylist.forFileID(uriParts.fileId, {
			appKey: this._options.appKey,
			drive: this._drive,
			sheets: this._sheets
		});
		return await gdbPlaylist.moveItems(index, count, newIndex);
	}
}

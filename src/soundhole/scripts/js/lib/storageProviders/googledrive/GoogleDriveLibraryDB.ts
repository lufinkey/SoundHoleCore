
import GoogleSheetsDBWrapper from './GoogleSheetsDBWrapper';
import {
	GoogleSheetsDB,
	GoogleSheetsDB$InstantiateOptions,
	GSDBInfo,
	GSDBRow,
	GSDBCellValue, 
	GoogleSheetsDB$InstantiateResult } from 'google-sheets-db';
import {
	drive_v3,
	sheets_v4 } from 'googleapis';
import {
	FollowedItem,
	FollowedItemRow,
	KeyingOptions } from '../StorageProvider';
import { GoogleSheetPage } from './types';

type RequiredGoogleAPIs = {
	drive: drive_v3.Drive
	sheets: sheets_v4.Sheets
}

type PrepareOptions = KeyingOptions & RequiredGoogleAPIs & {
	folderId: string
}

export default class GoogleDriveLibraryDB extends GoogleSheetsDBWrapper {
	static FILE_NAME = 'library';
	static VERSIONS = [ '1.0' ];
	static LATEST_VERSION = '1.0';

	static key_appProperties_userLibrary({appKey}: KeyingOptions): string {
		return `${appKey}_user_library`;
	}
	static key_devMetadata_libraryVersion({appKey}: KeyingOptions): string {
		return `${appKey}_db_version`;
	}
	static get TABLENAME_FOLLOWED_PLAYLISTS(): string {
		return 'followed_playlists';
	}
	static get TABLENAME_FOLLOWED_USERS(): string {
		return 'followed_users';
	}
	static get TABLENAME_FOLLOWED_ARTISTS(): string {
		return 'followed_artists';
	}
	static dbInfo({ appKey }: KeyingOptions): GSDBInfo {
		return {
			tables: [
				{ // Followed Playlists
					name: this.TABLENAME_FOLLOWED_PLAYLISTS,
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
							name: 'addedAt',
							type: 'timestamp',
							nonnull: true
						}
					],
					rowMetadataInfo: [
						{
							name: 'uri',
							type: 'rawstring'
						}
					],
					metadata: {}
				},
				{ // Followed Users
					name: this.TABLENAME_FOLLOWED_USERS,
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
							name: 'addedAt',
							type: 'timestamp',
							nonnull: true
						}
					],
					rowMetadataInfo: [
						{
							name: 'uri',
							type: 'rawstring'
						}
					],
					metadata: {}
				},
				{ // Followed Artists
					name: this.TABLENAME_FOLLOWED_ARTISTS,
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
							name: 'addedAt',
							type: 'timestamp',
							nonnull: true
						}
					],
					rowMetadataInfo: [
						{
							name: 'uri',
							type: 'rawstring'
						}
					],
					metadata: {}
				}
			],
			metadata: {
				[this.key_devMetadata_libraryVersion({appKey})]: this.LATEST_VERSION
			}
		};
	}



	static forFile(fileOrFileId: string | drive_v3.Schema$File, {appKey, drive, sheets}: KeyingOptions & RequiredGoogleAPIs): GoogleDriveLibraryDB {
		const fileId = (typeof fileOrFileId === 'string') ? fileOrFileId : fileOrFileId.id;
		if(!fileId) {
			throw new Error("Missing file ID");
		}
		const file = (typeof fileOrFileId === 'object') ? fileOrFileId : undefined;
		return new GoogleDriveLibraryDB({
			appKey,
			db: new GoogleSheetsDB({
				fileId,
				drive,
				sheets
			}),
			file
		});
	}

	static async findFile(folderId: string, {appKey, drive}: KeyingOptions & {drive: drive_v3.Drive}): Promise<drive_v3.Schema$File | null> {
		const libraryDBPropKey = this.key_appProperties_userLibrary({appKey});
		// find existing library file if exists
		const { files } = (await drive.files.list({
			q: `mimeType = 'application/vnd.google-apps.spreadsheet' and name = '${this.FILE_NAME}' and appProperties has { key='${libraryDBPropKey}' and value='true' } and '${folderId}' in parents and trashed = false`,
			fields: "*",
			spaces: 'drive',
			pageSize: 1
		})).data;
		if(files == null) {
			throw new Error(`Unable to find files property in response when searching for "${this.FILE_NAME}" file`);
		}
		const file = files[0];
		if(!file) {
			return null;
		}
		return file;
	}

	static async prepare({appKey, folderId, drive, sheets}: PrepareOptions): Promise<GoogleDriveLibraryDB> {
		// find existing library file if exists
		const file = await this.findFile(folderId, { appKey, drive });
		const existingFileId = file?.id;
		// create db object
		const db = new GoogleSheetsDB({
			fileId: existingFileId,
			drive,
			sheets
		});
		// instantiate DB if needed
		let instantiateResult: GoogleSheetsDB$InstantiateResult | null = null;
		if(existingFileId == null) {
			try {
				instantiateResult = await this._instantiateDB(db, {
					folderId,
					appKey
				});
			}
			catch(error) {
				if(db.fileId == null) {
					throw error;
				}
			}
		}
		// create library DB
		const libraryDB = new GoogleDriveLibraryDB({
			appKey,
			file: file ?? undefined,
			...(instantiateResult ?? {}),
			db
		});
		// fetch library DB content if file was already created
		if(existingFileId != null) {
			try {
				await libraryDB.fetchDBInfo();
			}
			catch(error) {}
		}
		return libraryDB;
	}



	private static async _instantiateDB(db: GoogleSheetsDB, options: {folderId?: string, file?: drive_v3.Schema$File, appKey: string}): Promise<GoogleSheetsDB$InstantiateResult> {
		const cls = GoogleDriveLibraryDB;
		const libraryDBPropKey = cls.key_appProperties_userLibrary({ appKey: options.appKey });
		const instOptions: GoogleSheetsDB$InstantiateOptions = {
			name: cls.FILE_NAME,
			privacy: 'private',
			db: cls.dbInfo({ appKey: options.appKey }),
			file: options.file
		};
		if(db.fileId == null) {
			const folderId = options?.folderId;
			if(folderId == null) {
				throw new Error(`No folder ID specified and no library file ID available`);
			}
			instOptions.file = {
				appProperties: {
					[libraryDBPropKey]: 'true'
				},
				parents: [folderId]
			};
		}
		return await db.instantiate(instOptions);
	}
	
	async instantiateIfNeeded() {
		if(!this.db.isInstantiated) {
			const cls = GoogleDriveLibraryDB;
			await cls._instantiateDB(this.db, {
				appKey: this.appKey,
				file: this.file ?? undefined
			});
		}
	}



	// Followed Items

	private async _getFollowedItemsInRange(tableName: string, {offset, limit}: {offset: number, limit: number}): Promise<GoogleSheetPage<FollowedItemRow>> {
		await this.instantiateIfNeeded();
		const tableData = await this.db.getTableData(tableName, {
			offset,
			limit
		});
		this.updateTableInfo(tableData);
		return {
			offset,
			total: tableData.rowCount,
			items: tableData.rows.map((row: GSDBRow, index): FollowedItemRow => {
				const item: any = {};
				for(let i=0; i<tableData.columnInfo.length; i++) {
					const column = tableData.columnInfo[i];
					item[column.name] = row.values[i];
				}
				item.index = offset + index;
				return item;
			})
		};
	}

	private async _checkFollowedItems(tableName: string, uris: string[]): Promise<boolean[]> {
		await this.instantiateIfNeeded();
		// get table info
		const tableInfo = await this.getTableInfo(tableName);
		// find matches
		const { matches } = await this.db.findTableRowsWithMetadata(tableInfo, uris.map((uri) => {
			return {
				key: 'uri',
				value: uri,
				matching: 'any'
			}
		}));
		// get results
		const results: boolean[] = [];
		for(const uri of uris) {
			const matchIndex = matches.findIndex((m) => (m.metadata.uri === uri));
			const followed: boolean = (matchIndex !== -1);
			results.push(followed);
			if(followed) {
				matches.splice(matchIndex, 1);
			}
		}
		return results;
	}

	private async _getFollowedItemsWithURIs(tableName: string, uris: string[]): Promise<FollowedItemRow[]> {
		await this.instantiateIfNeeded();
		// get table info
		const tableInfo = await this.getTableInfo(tableName);
		// find matches
		const { matches } = await this.db.getTableRowsWithMetadata(tableInfo, uris.map((uri) => {
			return {
				key: 'uri',
				value: uri,
				matching: 'any'
			}
		}));
		return matches.map((match) => {
			const { uri, provider, addedAt } = match.metadata;
			if(!uri) {
				throw new Error("missing uri property in row");
			}
			if(!provider) {
				throw new Error("missing provider property in row");
			}
			if(!addedAt) {
				throw new Error("missing adedAt property in row");
			}
			return {
				uri: uri as string,
				provider: provider as string,
				addedAt: addedAt as number,
				index: match.index
			};
		});
	}

	private async _addFollowedItems(tableName: string, items: FollowedItem[]): Promise<FollowedItemRow[]> {
		await this.instantiateIfNeeded();
		// get table info
		const tableInfo = await this.getTableInfo(tableName);
		// check if any of the items are already followed
		const alreadyFollowedItems = await this._getFollowedItemsWithURIs(tableName, items.map((item) => (item.uri)));
		// determine which items are already added and which ones need to be added
		const newItems: FollowedItem[] = [];
		const existingItems: FollowedItemRow[] = [];
		for(let i=0; i<items.length; i++) {
			const item = items[i];
			const followedIndex = alreadyFollowedItems.findIndex((p) => (p.uri === item.uri));
			if(followedIndex === -1) {
				// playlist is not already followed
				newItems.push(item);
			} else {
				// playlist is already followed
				existingItems.push(alreadyFollowedItems[followedIndex]);
				alreadyFollowedItems.splice(followedIndex, 1);
				i--;
			}
		}
		existingItems.sort((a, b) => (a.index - b.index));
		// insert new item rows
		this.db.insertTableRows(tableInfo, 0, newItems.map((item) => {
			const values: GSDBCellValue[] = [];
			for(const columnInfo of tableInfo.columnInfo) {
				values.push((item as any)[columnInfo.name]);
			}
			const metadata: {[key: string]: GSDBCellValue} = {};
			for(const metadataInfo of tableInfo.rowMetadataInfo) {
				metadata[metadataInfo.name] = (item as any)[metadataInfo.name];
			}
			return {
				values,
				metadata,
			};
		}));
		tableInfo.rowCount += newItems.length;
		// return followed item rows
		return [
			...newItems.map((item, index): FollowedItemRow => {
				return {
					uri: item.uri,
					provider: item.provider,
					addedAt: item.addedAt,
					index
				};
			}),
			...existingItems.map((item) => {
				return {
					uri: item.uri,
					provider: item.provider,
					addedAt: item.addedAt,
					index: item.index + newItems.length
				};
			})
		];
	}

	private async _removeFollowedItems(tableName: string, uris: string[]): Promise<void> {
		await this.instantiateIfNeeded();
		// get table info
		const tableInfo = await this.getTableInfo(tableName);
		// delete rows
		const { indexes } = await this.db.deleteTableRowsWithMetadata(tableInfo, uris.map((uri) => {
			return {
				key: 'uri',
				value: uri,
				matching: 'any'
			}
		}));
		tableInfo.rowCount -= indexes.length;
		if(tableInfo.rowCount < 0) {
			tableInfo.rowCount = 0;
		}
	}



	// Followed Playlists

	async getFollowedPlaylistsInRange(options: {offset: number, limit: number}): Promise<GoogleSheetPage<FollowedItemRow>> {
		const cls = GoogleDriveLibraryDB;
		return await this._getFollowedItemsInRange(cls.TABLENAME_FOLLOWED_PLAYLISTS, options);
	}
	
	async checkFollowedPlaylists(uris: string[]): Promise<boolean[]> {
		const cls = GoogleDriveLibraryDB;
		return await this._checkFollowedItems(cls.TABLENAME_FOLLOWED_PLAYLISTS, uris);
	}

	async getFollowedPlaylistsWithURIs(uris: string[]): Promise<FollowedItemRow[]> {
		const cls = GoogleDriveLibraryDB;
		return await this._getFollowedItemsWithURIs(cls.TABLENAME_FOLLOWED_PLAYLISTS, uris);
	}

	async addFollowedPlaylists(playlists: FollowedItem[]): Promise<FollowedItemRow[]> {
		const cls = GoogleDriveLibraryDB;
		return await this._addFollowedItems(cls.TABLENAME_FOLLOWED_PLAYLISTS, playlists);
	}

	async removeFollowedPlaylists(uris: string[]): Promise<void> {
		const cls = GoogleDriveLibraryDB;
		return await this._removeFollowedItems(cls.TABLENAME_FOLLOWED_PLAYLISTS, uris);
	}



	// Followed Users

	async getFollowedUsersInRange(options: {offset: number, limit: number}): Promise<GoogleSheetPage<FollowedItemRow>> {
		const cls = GoogleDriveLibraryDB;
		return await this._getFollowedItemsInRange(cls.TABLENAME_FOLLOWED_USERS, options);
	}
	
	async checkFollowedUsers(uris: string[]): Promise<boolean[]> {
		const cls = GoogleDriveLibraryDB;
		return await this._checkFollowedItems(cls.TABLENAME_FOLLOWED_USERS, uris);
	}

	async getFollowedUsersWithURIs(uris: string[]): Promise<FollowedItemRow[]> {
		const cls = GoogleDriveLibraryDB;
		return await this._getFollowedItemsWithURIs(cls.TABLENAME_FOLLOWED_USERS, uris);
	}

	async addFollowedUsers(users: FollowedItem[]): Promise<FollowedItemRow[]> {
		const cls = GoogleDriveLibraryDB;
		return await this._addFollowedItems(cls.TABLENAME_FOLLOWED_USERS, users);
	}

	async removeFollowedUsers(uris: string[]): Promise<void> {
		const cls = GoogleDriveLibraryDB;
		return await this._removeFollowedItems(cls.TABLENAME_FOLLOWED_USERS, uris);
	}



	// Followed Artists

	async getFollowedArtistsInRange(options: {offset: number, limit: number}): Promise<GoogleSheetPage<FollowedItemRow>> {
		const cls = GoogleDriveLibraryDB;
		return await this._getFollowedItemsInRange(cls.TABLENAME_FOLLOWED_ARTISTS, options);
	}
	
	async checkFollowedArtists(uris: string[]): Promise<boolean[]> {
		const cls = GoogleDriveLibraryDB;
		return await this._checkFollowedItems(cls.TABLENAME_FOLLOWED_ARTISTS, uris);
	}

	async getFollowedArtistsWithURIs(uris: string[]): Promise<FollowedItemRow[]> {
		const cls = GoogleDriveLibraryDB;
		return await this._getFollowedItemsWithURIs(cls.TABLENAME_FOLLOWED_ARTISTS, uris);
	}

	async addFollowedArtists(playlists: FollowedItem[]): Promise<FollowedItemRow[]> {
		const cls = GoogleDriveLibraryDB;
		return await this._addFollowedItems(cls.TABLENAME_FOLLOWED_ARTISTS, playlists);
	}

	async removeFollowedArtists(uris: string[]): Promise<void> {
		const cls = GoogleDriveLibraryDB;
		return await this._removeFollowedItems(cls.TABLENAME_FOLLOWED_ARTISTS, uris);
	}
}

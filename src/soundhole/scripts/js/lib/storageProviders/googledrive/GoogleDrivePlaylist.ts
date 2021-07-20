
import {
	ImageData,
	KeyingOptions,
	PlaylistItem,
	PlaylistItemPage,
	PlaylistPrivacyId,
	validatePlaylistPrivacy } from '../StorageProvider';
import {
	GoogleSheetsDB,
	GoogleSheetsDB$InstantiateResult,
	GSDBCellValue,
	GSDBColumnInfo,
	GSDBFullInfo,
	GSDBInfo,
	GSDBRow,
	GSDBTableData,
	GSDBTableSheetInfo } from 'google-sheets-db';
import {
	drive_v3,
	sheets_v4 } from 'googleapis';
import { v1 as uuidv1 } from 'uuid';
import { determinePermissionsPrivacy } from './types';

type ConstructOptions = {
	appKey: string
	db: GoogleSheetsDB
	dbInfo?: GSDBFullInfo
	file?: drive_v3.Schema$File
	privacy?: PlaylistPrivacyId
}

type RequiredGoogleAPIs = {
	drive: drive_v3.Drive
	sheets: sheets_v4.Sheets
}

type NewOptions = KeyingOptions & RequiredGoogleAPIs & {
	dbInfo?: GSDBFullInfo
	file?: drive_v3.Schema$File
	privacy?: PlaylistPrivacyId
}

type CreateOptions = KeyingOptions & RequiredGoogleAPIs & {
	folderId: string

	name: string
	privacy?: PlaylistPrivacyId
	description?: string
	image?: ImageData
}

export default class GoogleDrivePlaylist {
	appKey: string
	db: GoogleSheetsDB;
	dbInfo: GSDBFullInfo | null;
	file: drive_v3.Schema$File | null;
	privacy: PlaylistPrivacyId | null;

	static VERSIONS = [ '1.0' ];
	static LATEST_VERSION = '1.0';

	static ITEM_ONLY_COLUMNS = [
		'addedAt',
		'addedBy',
		'addedById'
	];

	static key_appProperties_playlistUUID({ appKey }: KeyingOptions) {
		return `${appKey}_playlist_uuid`;
	}
	static key_appProperties_imageData({ appKey }: KeyingOptions) {
		return `${appKey}_img_data`;
	}
	static key_appProperties_imageMimeType({ appKey }: KeyingOptions) {
		return `${appKey}_img_mimetype`;
	}
	static key_devMetadata_playlistVersion({ appKey }: KeyingOptions): string {
		return `${appKey}_playlist_version`;
	}
	static key_devMetdata_playlistItemId(): string {
		return `uniqueID`;
	}
	static db_tableName_items(): string {
		return 'items';
	}
	static dbInfo({ appKey }: KeyingOptions): GSDBInfo {
		return {
			tables: [
				{ // items table
					name: this.db_tableName_items(),
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
							name: this.key_devMetdata_playlistItemId(),
							type: 'rawstring'
						}
					],
					metadata: {}
				},
			],
			metadata: {
				[this.key_devMetadata_playlistVersion({ appKey })]: this.LATEST_VERSION
			}
		};
	}



	constructor(options: ConstructOptions) {
		if(!options.db.fileId) {
			throw new Error("db is missing required fileId");
		}
		this.appKey = options.appKey;
		this.db = options.db;
		this.dbInfo = options.dbInfo ?? null;
		this.file = options.file ?? null;
		this.privacy = options.privacy ?? (options.file?.permissions ? determinePermissionsPrivacy(options.file.permissions) : null);
	}

	get fileId(): string {
		return this.db.fileId as string;
	}



	static forFileID(fileId: string, options: NewOptions) {
		return new GoogleDrivePlaylist({
			appKey: options.appKey,
			db: new GoogleSheetsDB({
				fileId,
				drive: options.drive,
				sheets: options.sheets
			}),
			dbInfo: options.dbInfo,
			file: options.file,
			privacy: options.privacy
		});
	}



	static async create(options: CreateOptions): Promise<GoogleDrivePlaylist> {
		const { appKey } = options;
		const playlistUUIDKey = this.key_appProperties_playlistUUID({appKey});
		const imageDataKey = this.key_appProperties_imageData({appKey});
		const imageMimeTypeKey = this.key_appProperties_imageMimeType({appKey});
		const dbInfo = this.dbInfo({ appKey });
		// validate input
		if(options.privacy != null) {
			validatePlaylistPrivacy(options.privacy);
		}
		// prepare appProperties
		const appProperties: {[key: string]: string} = {};
		appProperties[playlistUUIDKey] = uuidv1();
		if(options.image) {
			appProperties[imageDataKey] = options.image.data;
			appProperties[imageMimeTypeKey] = options.image.mimeType;
		}
		// create playlist db
		const db = new GoogleSheetsDB({
			drive: options.drive,
			sheets: options.sheets
		});
		// instantiate db
		let instResult: GoogleSheetsDB$InstantiateResult;
		try {
			instResult = await db.instantiate({
				name: options.name,
				file: {
					description: options.description,
					parents: [ options.folderId ],
					appProperties: appProperties
				},
				privacy: options.privacy ?? 'private',
				db: dbInfo
			});
		} catch(error) {
			if(db.fileId != null) {
				// try to destroy the db if we can
				db.delete().catch((error) => {
					console.error(error);
				});
			}
			throw error;
		}
		const { file, privacy, dbInfo: dbFullInfo } = instResult;
		if(file.id == null) {
			throw new Error("Missing \"id\" property of newly created playlist file");
		}
		// return playlist
		return new GoogleDrivePlaylist({
			appKey,
			db,
			dbInfo: dbFullInfo,
			file,
			privacy
		});
	}



	async delete(options: {permanent?: boolean}): Promise<void> {
		if(options.permanent) {
			// delete file
			await this.db.delete();
		} else {
			// move file to trash
			await this.db.moveToTrash();
			if(this.file) {
				this.file.trashed = true;
			}
		}
	}

	async userCanEdit(user: drive_v3.Schema$User): Promise<boolean> {
		let done = false;
		let pageToken: string | null | undefined = null;
		const editRoles = ['writer','fileOrganizer','organizer','owner'];
		while(!done) {
			const { permissions, nextPageToken } = (await this.db.drive.permissions.list({
				fileId: this.fileId,
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
				else if(p.type === 'user'
					&& ((p.emailAddress != null && p.emailAddress === user.emailAddress)
						|| (p.id != null && p.id === user.permissionId))) {
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

	async getFile(): Promise<drive_v3.Schema$File> {
		if(this.file != null) {
			return this.file;
		}
		return await this.fetchFile();
	}

	async fetchFile(): Promise<drive_v3.Schema$File> {
		const file = (await this.db.drive.files.get({
			fileId: this.fileId,
			fields: "*"
		})).data;
		this.file = file;
		if(file.permissions) {
			this.privacy = determinePermissionsPrivacy(file.permissions);
		}
		return file;
	}

	async fetchDBInfo(): Promise<GSDBFullInfo> {
		const dbInfo = await this.db.getDBInfo();
		this.dbInfo = dbInfo;
		return dbInfo;
	}

	async fetchTableInfo(tableName: string): Promise<GSDBTableSheetInfo> {
		const tableInfo = await this.db.getTableInfo(tableName);
		this.updateTableInfo(tableInfo);
		return tableInfo;
	}

	async getTableInfo(tableName: string): Promise<GSDBTableSheetInfo> {
		const tableInfo = this.dbInfo?.tables.find((t) => (t.name === tableName));
		if(tableInfo) {
			return tableInfo;
		}
		return await this.fetchTableInfo(tableName);
	}

	async fetchItems({ offset, limit }: { offset: number, limit: number }): Promise<PlaylistItemPage> {
		if(!Number.isInteger(offset) || offset < 0) {
			throw new Error("offset must be a positive integer");
		} else if(!Number.isInteger(limit) || limit <= 0) {
			throw new Error("limit must be a positive non-zero integer");
		}
		const itemsTableName = GoogleDrivePlaylist.db_tableName_items();
		if(offset === 0) {
			// get table info and item rows together
			const tableData = await this.db.getTableData(itemsTableName, {
				offset,
				limit
			});
			this.updateTableInfo(tableData);
			return this._parsePlaylistItemsTableData(tableData);
		} else {
			// get table info
			const tableInfo = await this.db.getTableInfo(itemsTableName);
			this.updateTableInfo(tableInfo);
			// if we're out of range, return an empty page
			if(offset >= tableInfo.rowCount) {
				return {
					total: tableInfo.rowCount,
					offset: tableInfo.rowCount,
					items: []
				};
			}
			// get items rows and spreadsheet properties
			const tableData = await this.db.getTableData(itemsTableName, {
				offset,
				limit
			});
			this.updateTableInfo(tableData);
			return this._parsePlaylistItemsTableData(tableData);
		}
	}


	async insertItems(index: number, items: PlaylistItem[]): Promise<PlaylistItemPage> {
		if(!Number.isInteger(index) || index < 0) {
			throw new Error("index must be a positive integer");
		}
		// get table info
		const tableInfo = await this.getTableInfo(GoogleDrivePlaylist.db_tableName_items());
		// ensure index is in range
		if(index > tableInfo.rowCount) {
			throw new Error(`item index ${index} is out of bounds`);
		}
		// insert rows
		await this.db.insertTableRows(tableInfo, index, items.map((item: PlaylistItem): GSDBRow => {
			return this._rowFromPlaylistItem(tableInfo, item);
		}));
		tableInfo.rowCount += items.length;
		return {
			offset: index,
			total: tableInfo.rowCount,
			items
		};
	}

	async appendItems(items: PlaylistItem[]): Promise<PlaylistItemPage> {
		// get table info
		const tableInfo = await this.getTableInfo(GoogleDrivePlaylist.db_tableName_items());
		// insert rows at end
		const index = tableInfo.rowCount;
		await this.db.insertTableRows(tableInfo, index, items.map((item: PlaylistItem): GSDBRow => {
			return this._rowFromPlaylistItem(tableInfo, item);
		}));
		tableInfo.rowCount += items.length;
		return {
			offset: index,
			total: tableInfo.rowCount,
			items
		};
	}

	async deleteItemsWithIDs(itemIds: string[]): Promise<{indexes: number[]}> {
		if(itemIds.length === 0) {
			return {
				indexes: []
			};
		}
		const itemsTableName = GoogleDrivePlaylist.db_tableName_items();
		const rowItemIdKey = GoogleDrivePlaylist.key_devMetdata_playlistItemId();
		// delete rows
		const result = await this.db.deleteTableRowsWithMetadata(itemsTableName, itemIds.map((itemId) => {
			return {
				key: rowItemIdKey,
				value: itemId,
				match: 'any'
			};
		}));
		// update table info if available
		const tableInfo = this.dbInfo?.tables.find((t) => (t.name === itemsTableName));
		if(tableInfo) {
			tableInfo.rowCount -= result.indexes.length;
			if(tableInfo.rowCount < 0) {
				tableInfo.rowCount = 0;
			}
		}
		// return result
		return result;
	}

	async moveItems(index: number, count: number, newIndex: number): Promise<void> {
		if(!Number.isInteger(index) || index < 0) {
			throw new Error("index must be a positive integer");
		}
		if(!Number.isInteger(count) || count <= 0) {
			throw new Error("count must be a positive non-zero integer");
		}
		if(!Number.isInteger(newIndex) || newIndex < 0) {
			throw new Error("insertBefore must be a positive integer");
		}
		const itemsTableName = GoogleDrivePlaylist.db_tableName_items();
		await this.db.moveTableRows(itemsTableName, index, count, newIndex);
	}



	updateTableInfo(table: GSDBTableSheetInfo) {
		const tableInfo: GSDBTableSheetInfo = {
			sheetId: table.sheetId,
			name: table.name,
			columnInfo: table.columnInfo,
			rowMetadataInfo: table.rowMetadataInfo,
			metadata: table.metadata,
			rowCount: table.rowCount
		};
		if(this.dbInfo == null) {
			this.dbInfo = {
				spreadsheetId: this.db.fileId as string,
				metadata: {},
				tables: [ tableInfo ]
			};
			return;
		}
		const tableIndex = this.dbInfo.tables.findIndex((table) => (table.name === tableInfo.name));
		if(tableIndex === -1) {
			this.dbInfo.tables.push(tableInfo);
		} else {
			this.dbInfo.tables[tableIndex] = tableInfo;
		}
	}

	formatDate(date: Date): string {
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
		const playlistItemIdKey = GoogleDrivePlaylist.key_devMetdata_playlistItemId();
		const item: any = {};
		const track: any = {};
		for(let i=0; i<columns.length; i++) {
			const column = columns[i];
			const value = row.values[i];
			if(GoogleDrivePlaylist.ITEM_ONLY_COLUMNS.indexOf(column.name) !== -1) {
				item[column.name] = value;
			} else {
				track[column.name] = value;
			}
		}
		const itemId = row.metadata[playlistItemIdKey];
		if(itemId == null) {
			throw new Error(`Missing metadata property ${playlistItemIdKey} in row ${index}`);
		}
		item.uniqueId = itemId;
		if(track.type == null) {
			track.type = 'track';
		}
		item.track = track;
		if(item.addedAt != null) {
			// format timestamp as date string
			item.addedAt = this.formatDate(new Date(item.addedAt));
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

	_rowFromPlaylistItem(tableInfo: GSDBTableSheetInfo, item: PlaylistItem): GSDBRow {
		const rowItemIdKey = GoogleDrivePlaylist.key_devMetdata_playlistItemId();
		const values: GSDBCellValue[] = [];
		for(const columnInfo of tableInfo.columnInfo) {
			let value: GSDBCellValue;
			if(GoogleDrivePlaylist.ITEM_ONLY_COLUMNS.includes(columnInfo.name)) {
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
	}
}

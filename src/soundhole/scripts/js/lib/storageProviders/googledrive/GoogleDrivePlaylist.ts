
import GoogleSheetsDBWrapper from './GoogleSheetsDBWrapper';
import {
	ImageData,
	KeyingOptions,
	PlaylistItem,
	PlaylistItemPage,
	PlaylistItemProtection,
	PlaylistPrivacyId,
	Track,
	User,
	validatePlaylistPrivacy } from '../StorageProvider';
import {
	GoogleSheetsDB,
	GoogleSheetsDB$InstantiateResult,
	GSDBCellValue,
	GSDBFullInfo,
	GSDBInfo,
	GSDBNewRow,
	GSDBRow,
	GSDBTableData,
	GSDBTableInfo,
	GSDBTableSheetInfo } from 'google-sheets-db';
import {
	drive_v3,
	sheets_v4 } from 'googleapis';
import { v1 as uuidv1 } from 'uuid';

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

export type NewItemProtection = {
	description: string | null
	userIds?: string[]
}

export type InsertingPlaylistItem = {
	uniqueId: string
	track: Track
	addedAt: string | number
	addedBy: User
	addedById: string
	protection?: NewItemProtection
}

export default class GoogleDrivePlaylist extends GoogleSheetsDBWrapper {
	static VERSIONS = [ '1.0' ];
	static LATEST_VERSION = '1.0';

	static ITEM_ONLY_COLUMNS = [
		'uniqueId',
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
	static get TABLENAME_ITEMS(): string {
		return 'items';
	}
	static dbInfo({ appKey }: KeyingOptions): GSDBInfo {
		return {
			tables: [
				{ // items table
					name: this.TABLENAME_ITEMS,
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
							name: 'addedBy',
							type: 'any',
							nonnull: true
						}
					],
					rowMetadataInfo: [
						{
							name: 'addedById',
							type: 'rawstring'
						},
						{
							name: 'uniqueId',
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
				this._deleteDBUntilSuccess(db);
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



	private static async _deleteDBUntilSuccess(db: GoogleSheetsDB) {
		while(true) {
			try {
				await db.delete({permanent:true});
				return;
			} catch(error) {
				console.error(error);
			}
			// try again after 5 seconds
			await new Promise((resolve,reject) => {
				setTimeout(resolve, 5);
			});
		}
	}



	async delete(options: {permanent?: boolean}): Promise<void> {
		await this.db.delete(options);
		if(!options.permanent && this.file) {
			this.file.trashed = true;
		}
	}



	async fetchItems({ offset, limit }: { offset: number, limit: number }): Promise<PlaylistItemPage> {
		if(!Number.isInteger(offset) || offset < 0) {
			throw new Error("offset must be a positive integer");
		} else if(!Number.isInteger(limit) || limit <= 0) {
			throw new Error("limit must be a positive non-zero integer");
		}
		const itemsTableName = GoogleDrivePlaylist.TABLENAME_ITEMS;
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


	async insertItems(index: number, items: InsertingPlaylistItem[]): Promise<PlaylistItemPage> {
		if(!Number.isInteger(index) || index < 0) {
			throw new Error("index must be a positive integer");
		}
		// get table info
		const tableInfo = await this.getTableInfo(GoogleDrivePlaylist.TABLENAME_ITEMS);
		// ensure index is in range
		if(index > tableInfo.rowCount) {
			throw new Error(`item index ${index} is out of bounds`);
		}
		// insert rows
		const insertedRows = await this.db.insertTableRows(tableInfo, index, items.map((item): GSDBNewRow => {
			return this._rowFromPlaylistItem(tableInfo, item);
		}));
		tableInfo.rowCount += items.length;
		return {
			offset: index,
			total: tableInfo.rowCount,
			items: insertedRows.map((row, i) => {
				return this._parsePlaylistItemTableRow(row, tableInfo, (index + i));
			})
		};
	}

	async appendItems(items: InsertingPlaylistItem[]): Promise<PlaylistItemPage> {
		// get table info
		const tableInfo = await this.getTableInfo(GoogleDrivePlaylist.TABLENAME_ITEMS);
		// insert rows at end
		const index = tableInfo.rowCount;
		const insertedRows = await this.db.insertTableRows(tableInfo, index, items.map((item): GSDBNewRow => {
			return this._rowFromPlaylistItem(tableInfo, item);
		}));
		tableInfo.rowCount += items.length;
		return {
			offset: index,
			total: tableInfo.rowCount,
			items: insertedRows.map((row, i) => {
				return this._parsePlaylistItemTableRow(row, tableInfo, (index + i));
			})
		};
	}

	async deleteItemsWithIDs(itemIds: string[]): Promise<{indexes: number[]}> {
		if(itemIds.length === 0) {
			return {
				indexes: []
			};
		}
		const itemsTableName = GoogleDrivePlaylist.TABLENAME_ITEMS;
		// delete rows
		const result = await this.db.deleteTableRowsWithMetadata(itemsTableName, itemIds.map((itemId) => {
			return {
				key: 'uniqueId',
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

	async updateItemProtections(itemProtections: PlaylistItemProtection[]) {
		await this.db.updateRowProtections(itemProtections.map((p) => {
			let id: number | string = Number.parseInt(p.id);
			if(Number.isNaN(id)) {
				id = p.id;
			}
			return {
				id: id as number,
				description: p.description,
				userIds: p.userIds
			};
		}));
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
		const itemsTableName = GoogleDrivePlaylist.TABLENAME_ITEMS;
		await this.db.moveTableRows(itemsTableName, index, count, newIndex);
	}



	formatDate(date: Date): string {
		return date.toISOString();
	}

	_parsePlaylistItemTableRow(row: GSDBRow, tableInfo: GSDBTableInfo, index: number): PlaylistItem {
		if(row.values == null) {
			throw new Error("Missing values property for playlist row");
		}
		if(row.values.length < tableInfo.columnInfo.length) {
			throw new Error("Not enough columns in row");
		}
		let item: any = {};
		let track: any = {};
		for(let i=0; i<tableInfo.columnInfo.length; i++) {
			const column = tableInfo.columnInfo[i];
			const value = row.values[i];
			if(GoogleDrivePlaylist.ITEM_ONLY_COLUMNS.indexOf(column.name) !== -1) {
				item[column.name] = value;
			} else {
				track[column.name] = value;
			}
		}
		for(const metadataInfo of tableInfo.rowMetadataInfo) {
			const value = row.metadata[metadataInfo.name];
			if(GoogleDrivePlaylist.ITEM_ONLY_COLUMNS.indexOf(metadataInfo.name) !== -1) {
				item[metadataInfo.name] = value;
			} else {
				track[metadataInfo.name] = value;
			}
		}
		item.track = track;
		const playlistItem: PlaylistItem = item;
		if(playlistItem.uniqueId == null) {
			throw new Error(`Missing metadata property 'uniqueId' in row ${index}`);
		}
		if(playlistItem.track.type == null) {
			playlistItem.track.type = 'track';
		}
		if(playlistItem.addedAt != null) {
			// format timestamp as date string
			playlistItem.addedAt = this.formatDate(new Date(item.addedAt));
		}
		if(row.protection) {
			playlistItem.protection = {
				id: ''+row.protection.id,
				description: row.protection.description ?? null,
				userIds: row.protection.userIds
			};
		}
		return item;
	}

	_parsePlaylistItemsTableData(tableData: GSDBTableSheetInfo & GSDBTableData): PlaylistItemPage {
		const items: PlaylistItem[] = [];
		for(const row of tableData.rows) {
			const index = tableData.rowsOffset + items.length;
			items.push(this._parsePlaylistItemTableRow(row, tableData, index));
		}
		return {
			offset: tableData.rowsOffset,
			total: tableData.rowCount,
			items
		};
	}

	_rowFromPlaylistItem(tableInfo: GSDBTableSheetInfo, item: InsertingPlaylistItem): GSDBNewRow {
		const cls = GoogleDrivePlaylist;
		// build row values
		const values: GSDBCellValue[] = [];
		for(const columnInfo of tableInfo.columnInfo) {
			let value: GSDBCellValue;
			if(cls.ITEM_ONLY_COLUMNS.includes(columnInfo.name)) {
				value = (item as any)[columnInfo.name];
			} else {
				value = (item.track as any)[columnInfo.name];
			}
			values.push(value);
		}
		// build row metadata
		const metadata: {[key: string]: GSDBCellValue} = {};
		for(const metadataInfo of tableInfo.rowMetadataInfo) {
			let value: GSDBCellValue;
			if(cls.ITEM_ONLY_COLUMNS.includes(metadataInfo.name)) {
				value = (item as any)[metadataInfo.name];
			} else {
				value = (item.track as any)[metadataInfo.name];
			}
			metadata[metadataInfo.name] = value;
		}
		return {
			values,
			metadata,
			protection: item.protection
		};
	}
}

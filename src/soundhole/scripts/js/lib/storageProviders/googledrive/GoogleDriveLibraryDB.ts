
import GoogleSheetsDBWrapper from './GoogleSheetsDBWrapper';
import {
	GoogleSheetsDB,
	GoogleSheetsDB$InstantiateOptions,
	GSDBInfo,
	GSDBRow,
	GSDBCellValue } from 'google-sheets-db';
import {
	drive_v3,
	sheets_v4 } from 'googleapis';
import { KeyingOptions } from '../StorageProvider';

type RequiredGoogleAPIs = {
	drive: drive_v3.Drive
	sheets: sheets_v4.Sheets
}

type PrepareOptions = KeyingOptions & RequiredGoogleAPIs & {
	folderId: string
}

type FollowedPlaylist = {
	uri: string
	provider: string
	addedAt: number
}

type FollowedPlaylistRow = FollowedPlaylist & {
	index: number
}

type Page<T> = {
	total: number
	offset: number
	items: T[]
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
	static dbInfo({ appKey }: KeyingOptions): GSDBInfo {
		return {
			tables: [
				{
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
				}
			],
			metadata: {
				[this.key_devMetadata_libraryVersion({appKey})]: this.LATEST_VERSION
			}
		};
	}



	static forFileID(fileId: string, {appKey, drive, sheets}: KeyingOptions & RequiredGoogleAPIs): GoogleDriveLibraryDB {
		return new GoogleDriveLibraryDB({
			appKey,
			db: new GoogleSheetsDB({
				fileId,
				drive,
				sheets
			})
		});
	}

	static async prepare({appKey, folderId, drive, sheets}: PrepareOptions): Promise<GoogleDriveLibraryDB> {
		const libraryDBPropKey = this.key_appProperties_userLibrary({appKey});
		// find existing library file if exists
		const { files } = (await drive.files.list({
			q: `mimeType = 'application/vnd.google-apps.folder' and name = '${this.FILE_NAME}' and appProperties has { key='${libraryDBPropKey}' and value='true' } and '${folderId}' in parents and trashed = false`,
			fields: "*",
			spaces: 'drive',
			pageSize: 1
		})).data;
		if(files == null) {
			throw new Error(`Unable to find files property in response when searching for "${this.FILE_NAME}" file`);
		}
		const file = files[0];
		// create db object
		const db = new GoogleDriveLibraryDB({
			appKey,
			db: new GoogleSheetsDB({
				fileId: file?.id,
				drive,
				sheets
			}),
			file
		});
		// instantiate db if needed
		try {
			if(db.isInstantiated) {
				await db.fetchDBInfo();
			} else {
				await db.instantiateIfNeeded({
					folderId
				});
			}
		}
		catch(error) {
			if(db.fileId == null) {
				throw error;
			}
		}
		return db;
	}



	async instantiateIfNeeded(options?: {folderId?: string}) {
		const cls = GoogleDriveLibraryDB;
		const libraryDBPropKey = cls.key_appProperties_userLibrary({ appKey: this.appKey });
		if(!this.db.isInstantiated) {
			const instOptions: GoogleSheetsDB$InstantiateOptions = {
				name: cls.FILE_NAME,
				privacy: 'private',
				db: cls.dbInfo({ appKey: this.appKey }),
				file: undefined
			};
			if(this.db.fileId == null) {
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
			const { file, privacy, dbInfo } = await this.db.instantiate(instOptions);
			this.file = file;
			this.privacy = privacy;
			this.dbInfo = dbInfo;
		}
	}

	async getFollowedPlaylists({offset,limit}: {offset: number, limit: number}): Promise<Page<FollowedPlaylistRow>> {
		const cls = GoogleDriveLibraryDB;
		await this.instantiateIfNeeded();
		const tableData = await this.db.getTableData(cls.TABLENAME_FOLLOWED_PLAYLISTS, {
			offset,
			limit
		});
		this.updateTableInfo(tableData);
		return {
			offset,
			total: tableData.rowCount,
			items: tableData.rows.map((row: GSDBRow, index): FollowedPlaylistRow => {
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

	async findFollowedPlaylists(uris: string[]): Promise<FollowedPlaylistRow[]> {
		const cls = GoogleDriveLibraryDB;
		await this.instantiateIfNeeded();
		const tableInfo = await this.getTableInfo(cls.TABLENAME_FOLLOWED_PLAYLISTS);
		const { matches } = await this.db.getRowsWithMetadata(tableInfo, uris.map((uri) => {
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

	async addFollowedPlaylists(playlists: FollowedPlaylist[]): Promise<FollowedPlaylistRow[]> {
		const cls = GoogleDriveLibraryDB;
		await this.instantiateIfNeeded();
		// get table info
		const tableInfo = await this.getTableInfo(cls.TABLENAME_FOLLOWED_PLAYLISTS);
		// check if any of the playlists are already followed
		const alreadyFollowedPlaylists = await this.findFollowedPlaylists(playlists.map((p) => (p.uri)));
		// determine which playlists are already added and which ones need to be added
		const newPlaylists: FollowedPlaylist[] = [];
		const existingPlaylists: FollowedPlaylistRow[] = [];
		for(let i=0; i<playlists.length; i++) {
			const playlist = playlists[i];
			const followedIndex = alreadyFollowedPlaylists.findIndex((p) => (p.uri === playlist.uri));
			if(followedIndex === -1) {
				// playlist is not already followed
				newPlaylists.push(playlist);
			} else {
				// playlist is already followed
				existingPlaylists.push(alreadyFollowedPlaylists[followedIndex]);
				alreadyFollowedPlaylists.splice(followedIndex, 1);
				i--;
			}
		}
		existingPlaylists.sort((a, b) => (a.index - b.index));
		// insert new playlist rows
		this.db.insertTableRows(tableInfo, 0, newPlaylists.map((playlist): GSDBRow => {
			const values: GSDBCellValue[] = [];
			for(const columnInfo of tableInfo.columnInfo) {
				values.push((playlist as any)[columnInfo.name]);
			}
			const metadata: {[key: string]: GSDBCellValue} = {};
			for(const metadataInfo of tableInfo.rowMetadataInfo) {
				metadata[metadataInfo.name] = (playlist as any)[metadataInfo.name];
			}
			return {
				values,
				metadata
			};
		}));
		// return followed playlist rows
		return [
			...newPlaylists.map((playlist, index): FollowedPlaylistRow => {
				return {
					uri: playlist.uri,
					provider: playlist.provider,
					addedAt: playlist.addedAt,
					index
				};
			}),
			...existingPlaylists.map((playlist) => {
				return {
					uri: playlist.uri,
					provider: playlist.provider,
					addedAt: playlist.addedAt,
					index: playlist.index + newPlaylists.length
				};
			})
		];
	}
}

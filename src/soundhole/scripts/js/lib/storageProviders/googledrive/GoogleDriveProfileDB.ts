
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
import { KeyingOptions } from '../StorageProvider';
import {
	GoogleDriveProfileInfo,
	GoogleSheetPage } from './types';

type RequiredGoogleAPIs = {
	drive: drive_v3.Drive
	sheets: sheets_v4.Sheets
}

type PrepareOptions = KeyingOptions & RequiredGoogleAPIs & {
	folderId: string
}

type ProfileInfoRow = {
	key: string
	value: any
}

export default class GoogleDriveProfileDB extends GoogleSheetsDBWrapper {
	static FILE_NAME = 'profile';
	static VERSIONS = [ '1.0' ];
	static LATEST_VERSION = '1.0';

	static key_appProperties_userProfile({appKey}: KeyingOptions): string {
		return `${appKey}_user_profile`;
	}
	static key_devMetadata_profileVersion({appKey}: KeyingOptions): string {
		return `${appKey}_db_version`;
	}
	static get TABLENAME_PROFILE(): string {
		return 'profile';
	}

	static dbInfo({ appKey }: KeyingOptions): GSDBInfo {
		return {
			tables: [
				{ // Followed Playlists
					name: this.TABLENAME_PROFILE,
					columnInfo: [
						{
							name: 'key',
							type: 'rawstring',
							nonnull: true
						}, {
							name: 'value',
							type: 'any'
						}
					],
					rowMetadataInfo: [],
					metadata: {}
				}
			],
			metadata: {
				[this.key_devMetadata_profileVersion({appKey})]: this.LATEST_VERSION
			}
		};
	}



	static forFileID(fileId: string, {appKey, drive, sheets}: KeyingOptions & RequiredGoogleAPIs): GoogleDriveProfileDB {
		return new GoogleDriveProfileDB({
			appKey,
			db: new GoogleSheetsDB({
				fileId,
				drive,
				sheets
			})
		});
	}

	static async prepare({appKey, folderId, drive, sheets}: PrepareOptions): Promise<GoogleDriveProfileDB> {
		const profileDBPropKey = this.key_appProperties_userProfile({appKey});
		// find existing library file if exists
		const { files } = (await drive.files.list({
			q: `mimeType = 'application/vnd.google-apps.spreadsheet' and name = '${this.FILE_NAME}' and appProperties has { key='${profileDBPropKey}' and value='true' } and '${folderId}' in parents and trashed = false`,
			fields: "*",
			spaces: 'drive',
			pageSize: 1
		})).data;
		if(files == null) {
			throw new Error(`Unable to find files property in response when searching for "${this.FILE_NAME}" file`);
		}
		const file = files[0];
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
		const profileDB = new GoogleDriveProfileDB({
			appKey,
			file,
			...(instantiateResult ?? {}),
			db
		});
		// fetch library DB content if file was already created
		if(existingFileId != null) {
			try {
				await profileDB.fetchDBInfo();
			}
			catch(error) {}
		}
		return profileDB;
	}



	private static async _instantiateDB(db: GoogleSheetsDB, options: {folderId?: string, file?: drive_v3.Schema$File, appKey: string}): Promise<GoogleSheetsDB$InstantiateResult> {
		const cls = GoogleDriveProfileDB;
		const profileDBPropKey = cls.key_appProperties_userProfile({ appKey: options.appKey });
		const instOptions: GoogleSheetsDB$InstantiateOptions = {
			name: cls.FILE_NAME,
			privacy: 'public',
			db: cls.dbInfo({ appKey: options.appKey }),
			file: options.file
		};
		if(db.fileId == null) {
			const folderId = options?.folderId;
			if(folderId == null) {
				throw new Error(`No folder ID specified and no profile file ID available`);
			}
			instOptions.file = {
				appProperties: {
					[profileDBPropKey]: 'true'
				},
				parents: [folderId]
			};
		}
		return await db.instantiate(instOptions);
	}
	
	async instantiateIfNeeded() {
		if(!this.db.isInstantiated) {
			const cls = GoogleDriveProfileDB;
			await cls._instantiateDB(this.db, {
				appKey: this.appKey,
				file: this.file ?? undefined
			});
		}
	}



	// Profile Info

	async getProfileInfoRows(options: {offset?: number, limit?: number}): Promise<GoogleSheetPage<ProfileInfoRow>> {
		await this.instantiateIfNeeded();
		const cls = GoogleDriveProfileDB;
		let { offset, limit } = options;
		if(offset == null) {
			offset = 0;
		}
		const tableData = await this.db.getTableData(cls.TABLENAME_PROFILE, {
			offset,
			limit
		});
		this.updateTableInfo(tableData);
		return {
			offset,
			total: tableData.rowCount,
			items: tableData.rows.map((row: GSDBRow, index): ProfileInfoRow => {
				const item: any = {};
				for(let i=0; i<tableData.columnInfo.length; i++) {
					const column = tableData.columnInfo[i];
					item[column.name] = row.values[i];
				}
				item.index = (offset ?? 0) + index;
				return item;
			})
		};
	}

	async getProfileInfo(): Promise<GoogleDriveProfileInfo> {
		const page = await this.getProfileInfoRows({
			offset: 0,
			limit: 10000
		});
		// apply profile object
		const profileInfo: GoogleDriveProfileInfo = {};
		for(const row of page.items) {
			(profileInfo as any)[row.key] = row.value;
		}
		return profileInfo;
	}
}

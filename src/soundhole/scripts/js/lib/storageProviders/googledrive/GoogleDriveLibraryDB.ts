
import {
	GoogleSheetsDB,
	GoogleSheetsDB$InstantiateOptions,
	GSDBInfo } from 'google-sheets-db';
import {
	drive_v3,
	sheets_v4 } from 'googleapis';
import { KeyingOptions } from '../StorageProvider';

type ConstructOptions = {
	appKey: string
	db: GoogleSheetsDB
}

type RequiredGoogleAPIs = {
	drive: drive_v3.Drive
	sheets: sheets_v4.Sheets
}

type PrepareOptions = {
	appKey: string
	folderId: string
	drive: drive_v3.Drive,
	sheets: sheets_v4.Sheets
}

export default class GoogleDriveLibraryDB {
	appKey: string
	db: GoogleSheetsDB;

	static FILE_NAME = 'library';
	static VERSIONS = [ '1.0' ];
	static LATEST_VERSION = '1.0';

	static key_appProperties_userLibrary({appKey}: KeyingOptions): string {
		return `${appKey}_user_library`;
	}
	static key_devMetadata_libraryVersion({appKey}: KeyingOptions): string {
		return `${appKey}_db_version`;
	}
	static db_tableName_saved_playlist(): string {
		return 'saved_playlists';
	}
	static dbInfo({ appKey }: KeyingOptions): GSDBInfo {
		return {
			tables: [
				{
					name: this.db_tableName_saved_playlist(),
					columnInfo: [
						{
							name: 'uri',
							type: 'rawstring',
							nonnull: true
						},
						{
							name: 'parent',
							type: 'rawstring',
							nonnull: true
						}
					],
					rowMetadataInfo: [],
					metadata: {}
				}
			],
			metadata: {
				[this.key_devMetadata_libraryVersion({appKey})]: this.LATEST_VERSION
			}
		};
	}



	constructor({ appKey, db }: ConstructOptions) {
		this.appKey = appKey;
		this.db = db;
	}

	get isInstantiated(): boolean {
		return this.db.isInstantiated;
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
		// find existing library file
		const { files } = (await drive.files.list({
			q: `mimeType = 'application/vnd.google-apps.folder' and name = '${this.FILE_NAME}' and appProperties has { key='${libraryDBPropKey}' and value='true' } and '${folderId}' in parents and trashed = false`,
			fields: "nextPageToken, files(id, name)",
			spaces: 'drive',
			pageSize: 1
		})).data;
		if(files == null) {
			throw new Error(`Unable to find files property in response when searching for "${this.FILE_NAME}" file`);
		}
		let fileId = files[0]?.id;
		// instantiate DB if needed
		const db = new GoogleDriveLibraryDB({
			appKey,
			db: new GoogleSheetsDB({
				fileId,
				drive,
				sheets
			})
		});
		await db.instantiateIfNeeded({
			folderId
		});
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
			try {
				await this.db.instantiate(instOptions);
			} catch(error) {
				if(this.db.fileId == null) {
					throw error;
				}
			}
		}
	}
}

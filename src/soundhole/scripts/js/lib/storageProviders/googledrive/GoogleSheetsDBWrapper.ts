
import {
	GoogleSheetsDB,
	GSDBFullInfo,
	GSDBTableSheetInfo,
	GSDBPrivacy,
	determinePermissionsPrivacy
} from 'google-sheets-db';
import {
	drive_v3,
	sheets_v4
} from 'googleapis';
import { KeyingOptions } from '../StorageProvider';

export type GoogleSheetsDBWrapper$ConstructOptions = KeyingOptions & {
	db: GoogleSheetsDB
	file?: drive_v3.Schema$File
	dbInfo?: GSDBFullInfo
	privacy?: GSDBPrivacy
}

export default class GoogleSheetsDBWrapper {
	appKey: string
	db: GoogleSheetsDB
	file: drive_v3.Schema$File | null
	dbInfo: GSDBFullInfo | null
	privacy: GSDBPrivacy | null

	constructor({ appKey, db, file, dbInfo, privacy }: GoogleSheetsDBWrapper$ConstructOptions) {
		if(!db.fileId) {
			throw new Error("db is missing required fileId");
		}
		this.appKey = appKey;
		this.db = db;
		this.file = file ?? null;
		this.dbInfo = dbInfo ?? null;
		this.privacy = privacy ?? ((file?.permissions != null) ? determinePermissionsPrivacy(file.permissions) : null);
	}

	get fileId(): string {
		return this.db.fileId as string;
	}

	get isInstantiated(): boolean {
		return this.db.isInstantiated;
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
}

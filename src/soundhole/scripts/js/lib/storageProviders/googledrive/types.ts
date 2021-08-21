
import {
	drive_v3,
	sheets_v4,
	Auth } from 'googleapis';
import {
	MediaItemBuilder,
	createStorageProviderURI,
	parseStorageProviderURI } from '../StorageProvider';

export const LIBRARY_DB_NAME = 'library';
export const LIBRARY_DB_VERSIONS = [ '1.0' ];
export const LIBRARY_DB_LATEST_VERSION = '1.0';

export const PLAYLISTS_FOLDER_NAME = "Playlists";

export type GoogleDriveSession = {
	access_token: string,
	expiry_date: number,
	token_type: string
} & Auth.Credentials

export type GoogleDriveIdentity = drive_v3.Schema$About & {
	uri: string
	baseFolderId: string
}



export type GoogleSheetPage<T> = {
	total: number
	offset: number
	items: T[]
}



export type PlaylistIDParts = {
	fileId: string
	baseFolderId?: string | null
}

export const parsePlaylistID = (playlistId: string): PlaylistIDParts => {
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
};

export const createPlaylistID = (idParts: PlaylistIDParts): string => {
	if(!idParts.fileId) {
		throw new Error("missing fileId for createPlaylistID");
	}
	else if(!idParts.baseFolderId) {
		return idParts.fileId;
	}
	return `${idParts.baseFolderId}/${idParts.fileId}`;
};



export type PlaylistURIParts = {
	storageProvider: string
	type: string
	fileId: string
	baseFolderId?: string | null
}

export const parsePlaylistURI = (playlistURI: string, mediaItemBuilder: MediaItemBuilder | null) => {
	const uriParts = parseStorageProviderURI(playlistURI, mediaItemBuilder);
	const idParts = parsePlaylistID(uriParts.id);
	return {
		storageProvider: uriParts.storageProvider,
		type: uriParts.type,
		fileId: idParts.fileId,
		baseFolderId: idParts.baseFolderId
	};
};

export const createPlaylistURI = ({ storageProvider, type, fileId, baseFolderId }: PlaylistURIParts, mediaItemBuilder: MediaItemBuilder | null) => {
	return createStorageProviderURI({
		storageProvider,
		type: type,
		id: createPlaylistID({ fileId, baseFolderId })
	}, mediaItemBuilder);
}



export type UserIDParts = {
	email?: string
	permissionId?: string
	baseFolderId?: string | null
}

export const parseUserID = (userId: string): UserIDParts => {
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
};

export const createUserID = (idParts: UserIDParts): string => {
	const identifier = idParts.email ?? idParts.permissionId;
	if(!identifier) {
		throw new Error("missing identifier for _createUserID");
	}
	else if(!idParts.baseFolderId) {
		return identifier;
	}
	return `${idParts.baseFolderId}/${identifier}`;
};

export const createUserIDFromUser = (user: drive_v3.Schema$User, baseFolderId: string | null) => {
	return createUserID({
		email: user.emailAddress ?? undefined,
		permissionId: user.permissionId ?? undefined,
		baseFolderId
	});
};



export type UserURIParts = {
	storageProvider: string
	type: string
	email?: string
	permissionId?: string
	baseFolderId?: string | null
}

export const parseUserURI = (userURI: string, mediaItemBuilder: MediaItemBuilder | null): UserURIParts => {
	const uriParts = parseStorageProviderURI(userURI, mediaItemBuilder);
	const idParts = parseUserID(uriParts.id);
	return {
		storageProvider: uriParts.storageProvider,
		type: uriParts.type,
		email: idParts.email,
		permissionId: idParts.permissionId,
		baseFolderId: idParts.baseFolderId
	};
};

export const createUserURI = ({ storageProvider, type, email, permissionId, baseFolderId }: UserURIParts, mediaItemBuilder: MediaItemBuilder | null): string => {
	return createStorageProviderURI({
		storageProvider,
		type,
		id: createUserID({
			email,
			permissionId,
			baseFolderId
		})
	}, mediaItemBuilder);
};

export const createUserURIFromUser = ({storageProvider, user, baseFolderId}: {storageProvider: string, user: drive_v3.Schema$User, baseFolderId: string | null}, mediaItemBuilder: MediaItemBuilder | null): string => {
	return createStorageProviderURI({
		storageProvider,
		type: 'user',
		id: createUserIDFromUser(user, baseFolderId)
	}, mediaItemBuilder);
};



export type GoogleDriveProfileInfo = {
	spotifyUserURI?: string
	youtubeUserURI?: string
	bandcampUserURI?: string
}

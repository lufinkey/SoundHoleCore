
export type KeyingOptions = {
	appKey: string
}

export type Image = {
	url: string
	size: 'SMALL' | 'MEDIUM' | 'LARGE'
}

export type ImageData = {
	data: string
	mimeType: string
}

export type User = {
	partial: boolean
	type: 'user',
	provider: string
	name: string
	uri: string
	images: Image[]
}

export type Artist = {
	partial: boolean
	type: 'artist',
	provider: string
	name: string
	uri: string
	images: Image[]
}

export type Track = {
	type: string
	uri: string
	provider: string
	name: string
	albumName: string | null
	albumURI: string | null
	artists: Artist[]
	images: Image[]
	duration: number
}

export type PlaylistPrivacyId = 'private' | 'unlisted' | 'public'
export const PlaylistPrivacyValues: string[] = [ 'public', 'unlisted', 'private' ];
export const validatePlaylistPrivacy = (privacy: string): PlaylistPrivacyId => {
	if(!privacy || PlaylistPrivacyValues.indexOf(privacy) === -1) {
		throw new Error(`invalid privacy value "${privacy}" for google drive storage provider`);
	}
	return (privacy as PlaylistPrivacyId);
};


export type PlaylistItem = {
	uniqueId: string
	track: Track
	addedAt: string | number
	addedBy: User
}

export type Playlist = {
	partial: boolean
	type: 'playlist'
	uri: string
	name: string
	description: string
	versionId: string
	privacy: PlaylistPrivacyId
	owner: User
	itemCount?: number
	items?: PlaylistItem[] | {[key: number]: PlaylistItem}
}

export type PlaylistPage = {
	items: Playlist[]
	nextPageToken: string | null
}

export type PlaylistItemPage = {
	items: PlaylistItem[]
	offset: number
	total: number
}

export type CreatePlaylistOptions = {
	privacy?: PlaylistPrivacyId,
	description?: string
	image?: ImageData
}

export type GetPlaylistOptions = {
	itemsStartIndex?: number
	itemsLimit?: number
}

export type GetPlaylistItemsOptions = {
	offset: number,
	limit: number
}

export type GetUserPlaylistsOptions = {
	pageToken?: string
	pageSize?: number
}



export type URIParts = {
	storageProvider: string
	type: string
	id: string
}

export type MediaItemBuilder = {
	parseStorageProviderURI: (uri: string) => URIParts
	createStorageProviderURI: (storageProvider: string, type: string, id: string) => string
	readonly name: string
}

export const parseStorageProviderURI = (uri: string, mediaItemBuilder: MediaItemBuilder | null): URIParts => {
	if(!mediaItemBuilder || !mediaItemBuilder.parseStorageProviderURI) {
		const colonIndex1 = uri.indexOf(':');
		if(colonIndex1 === -1) {
			throw new Error("invalid URI "+uri);
		}
		const provider = uri.substring(0, colonIndex1);
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
	return mediaItemBuilder.parseStorageProviderURI(uri);
};

export const createStorageProviderURI = (uriParts: URIParts, mediaItemBuilder: MediaItemBuilder | null): string => {
	if(!mediaItemBuilder || !mediaItemBuilder.createStorageProviderURI) {
		return `${uriParts.storageProvider}:${uriParts.type}:${uriParts.id}`;
	}
	return mediaItemBuilder.createStorageProviderURI(uriParts.storageProvider, uriParts.type, uriParts.id);
}



export type NewFollowedItem = {
	uri: string
	provider: string
}

export type FollowedItem = {
	uri: string
	provider: string
	addedAt: number
}

export type FollowedItemRow = FollowedItem & {
	index: number
}



export default interface StorageProvider {
	get name(): string
	get displayName(): string

	createPlaylist(name: string, options: CreatePlaylistOptions): Promise<Playlist>
	getPlaylist(playlistURI: string, options: GetPlaylistOptions): Promise<Playlist>
	deletePlaylist(playlistURI: string): Promise<void>
	getMyPlaylists(options: GetUserPlaylistsOptions): Promise<PlaylistPage>
	getUserPlaylists(userURI: string, options: GetUserPlaylistsOptions): Promise<PlaylistPage>
	isPlaylistEditable(playlistURI: string): Promise<boolean>

	getPlaylistItems(playlistURI: string, options: GetPlaylistItemsOptions): Promise<PlaylistItemPage>
	insertPlaylistItems(playlistURI: string, index: number, tracks: Track[]): Promise<PlaylistItemPage>
	appendPlaylistItems(playlistURI: string, tracks: Track[]): Promise<PlaylistItemPage>
	deletePlaylistItems(playlistURI: string, itemIds: string[]): Promise<{indexes: number[]}>
	movePlaylistItems(playlistURI: string, index: number, count: number, insertBefore: number): Promise<void>

	followPlaylists(playlists: NewFollowedItem[]): Promise<FollowedItem[]>
	unfollowPlaylists(playlistURIs: string[]): Promise<void>

	followUsers(users: NewFollowedItem[]): Promise<FollowedItem[]>
	unfollowUsers(userURIs: string[]): Promise<void>

	followArtists(artists: NewFollowedItem[]): Promise<FollowedItem[]>
	unfollowArtists(artistURIs: string[]): Promise<void>
}

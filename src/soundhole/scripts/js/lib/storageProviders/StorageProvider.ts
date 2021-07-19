
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

export default interface StorageProvider {
	get canStorePlaylists(): boolean
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
}

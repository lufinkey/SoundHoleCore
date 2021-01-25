
class StorageProvider {
	get canStorePlaylists() {
		return false;
	}
	
	get name() {
		throw new Error("name is not implemented");
	}

	get displayName() {
		throw new Error("displayName is not implemented");
	}
	
	async createPlaylist(name, { privacy, description } = {}) {
		throw new Error("createPlaylist is not implemented");
	}

	async getPlaylist(playlistURI) {
		throw new Error("getPlaylist is not implemented");
	}

	async deletePlaylist(playlistURI) {
		throw new Error("deletePlaylist is not implemented");
	}

	async getMyPlaylists(options) {
		throw new Error("getMyPlaylists is not implemented");
	}

	async getUserPlaylists(userURI, options) {
		throw new Error("getUserPlaylists is not implemented");
	}

	async isPlaylistEditable(playlistURI) {
		throw new Error("isPlaylistEditable is not implemented");
	}

	async getPlaylistItems(playlistURI, options) {
		throw new Error("getPlaylistItems is not implemented");
	}

	async insertPlaylistItems(playlistURI, index, tracks) {
		throw new Error("insertPlaylistItems is not implemented");
	}

	async appendPlaylistItems(playlistURI, tracks) {
		throw new Error("appendPlaylistItems is not implemented");
	}

	async deletePlaylistItems(playlistURI, itemIds) {
		throw new Error("deletePlaylistItems is not implemented");
	}

	async reorderPlaylistItems(playlistURI, index, count, insertBefore) {
		throw new Error("reorderPlaylistItems is not implemented");
	}
}


module.exports = StorageProvider;


class StorageProvider {
	get canStorePlaylists() {
		return false;
	}
	
	async createPlaylist(name, options={}) {
		throw new Error("createPlaylist is not implemented");
	}

	async getPlaylist(playlistId) {
		throw new Error("getPlaylist is not implemented");
	}

	async deletePlaylist(playlistId) {
		throw new Error("deletePlaylist is not implemented");
	}

	async getPlaylists(options={}) {
		throw new Error("getPlaylists is not implemented");
	}

	async getPlaylistItems(options) {
		throw new Error("getPlaylistItems is not implemented");
	}
}

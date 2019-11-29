//
//  MediaProvider.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 8/18/19.
//  Copyright © 2019 Luis Finke. All rights reserved.
//

#include "MediaProvider.hpp"

namespace sh {
	Promise<$<Track>> MediaProvider::getTrack(String uri) {
		return getTrackData(uri).map<$<Track>>(nullptr, [=](auto data) {
			return Track::new$(this, data);
		});
	}

	Promise<$<Artist>> MediaProvider::getArtist(String uri) {
		return getArtistData(uri).map<$<Artist>>(nullptr, [=](auto data) {
			return Artist::new$(this, data);
		});
	}

	Promise<$<Album>> MediaProvider::getAlbum(String uri) {
		return getAlbumData(uri).map<$<Album>>(nullptr, [=](auto data) {
			return Album::new$(this, data);
		});
	}

	Promise<$<Playlist>> MediaProvider::getPlaylist(String uri) {
		return getPlaylistData(uri).map<$<Playlist>>(nullptr, [=](auto data) {
			return Playlist::new$(this, data);
		});
	}
}

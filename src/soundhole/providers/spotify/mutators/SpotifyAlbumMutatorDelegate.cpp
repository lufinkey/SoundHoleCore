//
//  SpotifyAlbumMutatorDelegate.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/15/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "SpotifyAlbumMutatorDelegate.hpp"
#include <soundhole/providers/spotify/SpotifyProvider.hpp>

namespace sh {
	SpotifyAlbumMutatorDelegate::SpotifyAlbumMutatorDelegate($<Album> album)
	: album(album) {
		//
	}

	Promise<void> SpotifyAlbumMutatorDelegate::loadItems(Mutator* mutator, size_t index, size_t count) {
		auto album = this->album.lock();
		auto provider = (SpotifyProvider*)album->mediaProvider();
		auto id = provider->idFromURI(album->uri());
		return provider->spotify->getAlbumTracks(id, {
			.market="from_token",
			.offset=index,
			.limit=count
		}).then([=](SpotifyPage<SpotifyTrack> page) -> void {
			auto items = page.items.map<$<AlbumItem>>([&](auto& track) {
				auto trackData = provider->createTrackData(track, true);
				trackData.albumName = album->name();
				trackData.albumURI = album->uri();
				return AlbumItem::new$(album, {
					.track=Track::new$(provider, trackData)
				});
			});
			mutator->lock([&]() {
				mutator->applyAndResize(page.offset, page.total, items);
			});
		});
	}
}

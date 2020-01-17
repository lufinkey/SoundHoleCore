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
			.offset=index,
			.limit=count
		}).then([=](SpotifyPage<SpotifyTrack> page) -> void {
			auto items = page.items.map<$<AlbumItem>>([&](auto& track) {
				return AlbumItem::new$(album, {
					.track=Track::new$(provider, provider->createTrackData(track))
				});
			});
			mutator->lock([&]() {
				mutator->resize(page.total);
				mutator->apply(page.offset, items);
			});
		});
	}
}

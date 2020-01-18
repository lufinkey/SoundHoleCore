//
//  SpotifyPlaylistMutatorDelegate.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/15/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "SpotifyPlaylistMutatorDelegate.hpp"
#include <soundhole/providers/spotify/SpotifyProvider.hpp>

namespace sh {
	SpotifyPlaylistMutatorDelegate::SpotifyPlaylistMutatorDelegate($<Playlist> playlist)
	: playlist(playlist) {
		//
	}

	Promise<void> SpotifyPlaylistMutatorDelegate::loadItems(Mutator* mutator, size_t index, size_t count) {
		auto playlist = this->playlist.lock();
		auto provider = (SpotifyProvider*)playlist->mediaProvider();
		auto id = provider->idFromURI(playlist->uri());
		return provider->spotify->getPlaylistTracks(id, {
			.offset=index,
			.limit=count
		}).then([=](SpotifyPage<SpotifyPlaylist::Item> page) -> void {
			auto items = page.items.map<$<PlaylistItem>>([&](auto& item) {
				return PlaylistItem::new$(playlist, provider->createPlaylistItemData(item));
			});
			mutator->lock([&]() {
				mutator->resize(page.total);
				mutator->apply(page.offset, items);
			});
		});
	}
}

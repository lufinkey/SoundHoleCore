//
//  SpotifyPlaylistMutatorDelegate.cpp
//  SoundHoleCore
//
//  Created by Luis Finke on 1/15/20.
//  Copyright © 2020 Luis Finke. All rights reserved.
//

#include "SpotifyPlaylistMutatorDelegate.hpp"
#include <soundhole/providers/spotify/SpotifyProvider.hpp>
#include <soundhole/database/MediaDatabase.hpp>

namespace sh {
	SpotifyPlaylistMutatorDelegate::SpotifyPlaylistMutatorDelegate($<Playlist> playlist)
	: playlist(playlist) {
		//
	}

	size_t SpotifyPlaylistMutatorDelegate::getChunkSize() {
		return 100;
	}

	Promise<void> SpotifyPlaylistMutatorDelegate::loadItems(Mutator* mutator, size_t index, size_t count, LoadItemOptions options) {
		auto playlist = this->playlist.lock();
		if(options.database == nullptr) {
			// online load
			auto provider = (SpotifyProvider*)playlist->mediaProvider();
			auto id = provider->idFromURI(playlist->uri());
			return provider->spotify->getPlaylistTracks(id, {
				.market="from_token",
				.offset=index,
				.limit=count
			}).then([=](SpotifyPage<SpotifyPlaylist::Item> page) -> void {
				auto items = page.items.map<$<PlaylistItem>>([&](auto& item) {
					return PlaylistItem::new$(playlist, provider->createPlaylistItemData(item));
				});
				mutator->lock([&]() {
					mutator->applyAndResize(page.offset, page.total, items);
				});
			});
		} else {
			// offline load
			return options.database->loadPlaylistItems(playlist, mutator, index, count);
		}
	}
}
